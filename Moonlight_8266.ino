/**
 * Moonlight_8266
 * Copyright 2020-2024 Winford (Uncle Grumpy) <winford@object.stream>
 *
 * Web controlled RGB LED interface
 *
 * SPDX-License-Identifier: MIT
 */

#include "Config.h"
#include "PrefsLib.h"

#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
#include <WiFiUdp.h>
#include <cmath>

#if defined __has_include && __has_include(<ESPAsyncWebServer.h>) && __has_include(<ESPAsyncTCP.h>)
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#define USE_ASYNC true
#else
#include <ESP8266WebServer.h>
#endif

// DO NOT CHANGE! If you need to invert LED values edit COMMON_ANODE value in Config.h
#define LED_MAX 1023
#define LED_OFF 0

// Set AP mode IP address
IPAddress apIP(192, 168, 4, 1);
// Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WiFiMulti wifiMulti;
// create a web server instance on port 80
#ifdef USE_ASYNC
AsyncWebServer server(80);
#else 
ESP8266WebServer server(80);
#endif
// create a websocket server instance on port 81
// TODO: update to use async if available
WebSocketsServer webSocket(81);
// create an instance of the DNSServer class, called 'dnsServer'
// TODO: update to use async if available
DNSServer dnsServer;

// For rainbow mode.
bool rainbow = false;
// keeping track of saved color preferences.
char saved_color[9];
// current color in HTML format.
char web_color[8];
// To show correct color on the moon during rainbow mode and restore last color when deactivated.
char rainbowState[8];

ADC_MODE(ADC_VCC);

/*___________________________________SETUP_______________________________________________________*/

void setup() {
  pinMode(LED_PIN_RED, OUTPUT); // the pins with LEDs connected are outputs
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_BLUE, OUTPUT);
  analogWriteRange(1023);
  analogWriteFreq(2000);

  if (LOG_ENABLED) {
    Serial.begin(115200); // Start the Serial communication to send messages to the computer
    delay(10);
    Serial.println("\r\n");
  }

  // Restore saved color settings.
  colorInit();
  // Start the LittleFS file system
  startLittleFS();
  // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for
  // either an AP or STA connection.
  startWiFi();
  // Start the DNS and mDNS responder
  startDNS();
  // Start a WebSocket server
  startServer();
  // Start the OTA service
  startWebSocket();
  // Start an HTTP server with a file read handler and an upload handler
  startOTA();
}

/*____________________________________LOOP_______________________________________________________*/

unsigned long prevMillis = millis();
int hue = 0;

void loop() {
  // measure vcc voltage at approx. 60 sec intervals. varies slightly under load, but it's not
  // critical.
  for (int i = 0; i <= 750000; i++) {
    // constantly check for websocket events
    webSocket.loop();
    // handle dns requests
    dnsServer.processNextRequest();
    // handle server requests
    server.handleClient();

    if (rainbow) {
      if (millis() > prevMillis + 27) {
        // Cycle through the color wheel in ~10 seconds (increment by one degree every 27 ms)
        if (++hue == 360) {
          hue = 0;
        }
        setHue(hue); // Set the RGB LED color
        prevMillis = millis();
      }
    }
    ArduinoOTA.handle(); // Check for OTA update.
  }
  batteryCheck();
}

/*________________________________SETUP_FUNCTIONS________________________________________________*/

// Start a Wi-Fi access point, and try to connect to some given access points.
// Then wait for either an AP or STA connection
void startWiFi() {
  // sets wifi power, 0 lowest 20 highest (only using integer values)
  uint8_t tx_pow_cfg[1] = { '\0' };
  float tx_pow;
  if (loadCfg(TX_POW_CFG, tx_pow_cfg)) {
    if (tx_pow_cfg[0] >= 21) {
      tx_pow = 20.5f;
    } else {
      tx_pow = 1.0f * tx_pow_cfg[0];
    }
    if (LOG_ENABLED) {
      Serial.printf("Using tx power %f from stored configuration\n", tx_pow);
    }
  } else {
    tx_pow = 1.0f * TX_POW;
    if (LOG_ENABLED) {
      Serial.printf("Using default tx power %f\n", tx_pow);
    }
  }
  WiFi.setOutputPower(tx_pow);

  char hostname_cfg[MAX_CHAR] = { '\0' };
  if (!loadCfg(HOSTNAME_CFG,  reinterpret_cast< uint8_t* >(hostname_cfg))) {
    memcpy(hostname_cfg, hostName, strlen(hostName));
    if (LOG_ENABLED) {
      Serial.printf("No hostname saved, using default: %s\n", hostName);
    }
  }
  char *hostname = (char*) malloc(sizeof(char) * strlen(hostname_cfg));
  memcpy(hostname, hostname_cfg, strlen(hostname_cfg));
  WiFi.hostname(hostname);
  free(hostname);

  char wl_ssid[MAX_CHAR] = { '\0' };
  char wl_psk[MAX_CHAR] = { '\0' };

  uint8_t start_ap_cfg[1] = { '\0' };
  bool start_ap = START_AP_DEFAULT;
  if (loadCfg(AP_ENABLED_CFG, start_ap_cfg)) {
    start_ap = static_cast<bool>(start_ap_cfg[0]);
    if (LOG_ENABLED) {
      Serial.printf("Using stored preference: Start AP %s\n", start_ap?"true":"false");
    }
  } else {
    if (LOG_ENABLED) {
      Serial.println("Starting AP by default");
    }
  }

  if (start_ap) {  
    if (!loadCfg(AP_SSID_CFG, reinterpret_cast< uint8_t* >(wl_ssid))) {
      if (LOG_ENABLED) {
        Serial.printf("No AP ssid saved, using default name %s\n", ap_ssid);
      }
      memcpy(wl_ssid, ap_ssid, strlen(ap_ssid));
    }
      
    if (!loadCfg(AP_PSK_CFG, reinterpret_cast< uint8_t* >(wl_psk))) {
      if (LOG_ENABLED) {
        Serial.println("No AP PSK saved, using default (open network)");
      }
      memcpy(wl_psk, ap_password, strlen(ap_password));
    }

    WiFi.softAP(wl_ssid, wl_psk); // Start the access point
  }

  uint8_t start_sta_cfg[1] = { '\0' };
  bool start_sta = false;
  if (!loadCfg(STA_ENABLED_CFG, start_sta_cfg)) {
    if (LOG_ENABLED) {
      start_sta = START_STA_DEFAULT;
      Serial.printf("No preference saved, using start STA default %s\n", start_sta?"true":"false");
    } 
  } else {
    start_sta = static_cast<bool>(start_sta_cfg[0]);
  }
  
  if (start_sta) {
    if (!loadCfg(STA_SSID_0_CFG, reinterpret_cast< uint8_t* >(wl_ssid))) {
      if (START_STA_DEFAULT) {
        start_sta = true;
        memcpy(wl_ssid, sta_ssid, strlen(sta_ssid));
      } else {
        if (LOG_ENABLED) {
          Serial.println("STA Mode enabled, but failed to retrieve ssid from configuration");
        }
      } 
    }

    if (!loadCfg(STA_PSK_0_CFG, reinterpret_cast< uint8_t* >(wl_psk))) {
      if (START_STA_DEFAULT) {
        memcpy(wl_psk, sta_password, strlen(sta_password));
      } else {
        wl_psk[0] = 0;
        if (LOG_ENABLED) {
          Serial.printf("No STA PSK saved, assuming open network for %s\n", wl_ssid);
        }
      }
    }

    if (start_sta) {
      wifiMulti.addAP(wl_ssid, wl_psk);
      if (LOG_ENABLED) {
        Serial.printf("Added WiFI multi configuration for network %s\n", wl_ssid);
      }
    }

    bool add_sta1 = false;
    if (!loadCfg(STA_SSID_1_CFG, reinterpret_cast< uint8_t* >(wl_ssid))) {
      if (LOG_ENABLED) {
        Serial.println("No additional networks configured for Station Mode.");
      }
    } else {
      add_sta1 = true;
    }

    if (!loadCfg(STA_PSK_1_CFG, reinterpret_cast< uint8_t* >(wl_psk))) {
      if (LOG_ENABLED) {
        Serial.printf("No STA PSK saved, assuming open network for %s\n", wl_ssid);
      }
      wl_psk[0] = 0;
    }

    if (add_sta1) {
      wifiMulti.addAP(wl_ssid, wl_psk);
      if (LOG_ENABLED) {
        Serial.printf("Added WiFI multi configuration for network %s\n", wl_ssid);
      }

      bool add_sta2 = false;
      if (!loadCfg(STA_SSID_2_CFG, reinterpret_cast< uint8_t* >(wl_ssid))) {
        if (LOG_ENABLED) {
          Serial.println("Third Station Mode network configuration is unused");
        }
      } else {
        add_sta2 = true;
      }

      if (!loadCfg(STA_PSK_2_CFG, reinterpret_cast< uint8_t* >(wl_psk))) {
        if (LOG_ENABLED) {
          Serial.printf("No STA PSK saved, assuming open network for %s\n", wl_ssid);
        }
        wl_psk[0] = 0;
      }

      if (add_sta2) {
        wifiMulti.addAP(wl_ssid, wl_psk);
        if (LOG_ENABLED) {
          Serial.printf("Added WiFI multi configuration for network %s\n", wl_ssid);
        }
      }
    }
  } // endif (start_sta)

  while (wifiMulti.run() != WL_CONNECTED && (!start_ap)) {  
    connecting_indicator();
  }
  if (!start_ap) {
    // Restore moon color
    htmlColorPWM(web_color);
  }

  if (LOG_ENABLED) {
    Serial.println("WiFI ready.");
  }
}

void startOTA() { // Start the OTA service
  char hostname[MAX_CHAR] = { '\0' };
  if (!loadCfg(HOSTNAME_CFG,  reinterpret_cast< uint8_t* >(hostname))) {
    memcpy(hostname, hostName, strlen(hostName));
    if (LOG_ENABLED) {
      Serial.printf("No hostname saved, using default: %s\n", hostname);
    }
  }

  ArduinoOTA.setHostname(hostname);

  char admin_pass[MAX_CHAR] = { '\0' };
  if (!loadCfg(ADMIN_PASSWORD_CFG,  reinterpret_cast< uint8_t* >(admin_pass))) {
    memcpy(admin_pass, DefaultPassword, strlen(DefaultPassword));
    if (LOG_ENABLED) {
      Serial.println("No admin password set, using default: <*****>");
    }
  }

  ArduinoOTA.setPasswordHash(admin_pass);
  ArduinoOTA.setPort(8266);
  
  ArduinoOTA.onStart([]() {
    if (LOG_ENABLED) {
      Serial.println("Starting OTA update.");
    }
  });

  ArduinoOTA.onEnd([]() {
    if (LOG_ENABLED) {
      Serial.println("Update complete");
    }
    setLed(LED_PIN_RED, LED_OFF);
    setLed(LED_PIN_GREEN, LED_MAX);
    setLed(LED_PIN_BLUE, LED_OFF);
    delay(3000);
    ESP.restart();
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (LOG_ENABLED) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    }
    // Start at full brightness and fade out until complete.
    int UploadProg = round(LED_MAX - ((progress / (total / 100)) * 10.2f));
    setLed(LED_PIN_RED, LED_OFF);
    setLed(LED_PIN_GREEN, LED_OFF);
    setLed(LED_PIN_BLUE, UploadProg);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    setLed(LED_PIN_BLUE, LED_OFF);
    setLed(LED_PIN_GREEN, LED_OFF);
    setLed(LED_PIN_RED, LED_MAX);
    if (LOG_ENABLED) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
    }
    delay(1000);
    for (int m = 0; m <= 8; m++) {
      setLed(LED_PIN_RED, LED_MAX);
      delay(33);
      setLed(LED_PIN_RED, 768);
      delay(100);
      setLed(LED_PIN_RED, 256);
      delay(200);
    }
    htmlColorPWM(web_color);
  });
  
  ArduinoOTA.begin();
  if (LOG_ENABLED) {
    Serial.println("OTA service ready.");
  }
}

void startLittleFS() {
  LittleFS.begin();
  if (LOG_ENABLED) {
    // List the file system contents
    Serial.println("LittleFS started. Contents:");
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\r\n");
  }
}

void startWebSocket() {
  webSocket.begin();
  // setup incoming websocket message callback function 'webSocketEvent'
  webSocket.onEvent(webSocketEvent); 
  MDNS.addService("ws", "tcp", 81);
  if (LOG_ENABLED) {
    Serial.println("WebSocket server started.");
  }
}

void startDNS() {
  char hostname[MAX_CHAR] = { '\0' };
  if (!loadCfg(HOSTNAME_CFG,  reinterpret_cast< uint8_t* >(hostname))) {
    memcpy(hostname, hostName, strlen(hostName));
    if (LOG_ENABLED) {
      Serial.printf("No hostname saved, using default: %s\n", hostname);
    }
  }

  // Setup mDNS hostname so http server can be added when started.
  MDNS.begin(hostname);

  uint8_t start_ap_cfg[1] = { '\0' };
  bool start_ap = START_AP_DEFAULT;
  if (loadCfg(AP_ENABLED_CFG, start_ap_cfg)) {
    start_ap = static_cast<bool>(start_ap_cfg[0]);
  }
  bool dns_active = false;
  if (start_ap) {
    dns_active = true;
    /* DNS Server only started if AP is enabled, otherwise only mDNS is used */
    // default 5 minute TTL.
    dnsServer.setTTL(300);
    if (WiFi.status() != WL_CONNECTED) {
      // AP only mode, start DNS server redirecting all requests to this host
      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer.start(53, "*", apIP);
    } else {
      // AP+STA mode, start DNS server redirecting hostname requests to assigned IP.
      dnsServer.start(53, hostname, WiFi.localIP());
    }   
  }

  if (LOG_ENABLED) {
    Serial.printf("mDNS responder started: http://%s.local\n", hostname);
    if (dns_active) {
      IPAddress ip = start_ap ? apIP : WiFi.localIP();
      Serial.printf("DNS responder started on %i.%i.%i.%i:53\n", ip[0], ip[1], ip[2], ip[3]);
    }
  }
}

void startServer() {
  // Start a HTTP server with a file read handler and an upload handler
  server.on(
    // If a POST request is sent to the /edit.html address go to 'handleFileUpload'
    "/edit.html", HTTP_POST,
    []() {                                
      server.send(200, "text/plain", "");
    },
    handleFileUpload);
  // Other page requests go to function 'handleNotFound' and serve the file if the file exists
  server.onNotFound(handleNotFound);
  // start the HTTP server
  server.begin();
  // Advertise http port 80 service over mdns.
  MDNS.addService("http", "tcp", 80);
  if (LOG_ENABLED) {
    Serial.println("HTTP server started.");
  }
}

void colorInit() {
  uint8_t rain_cfg[1] = { '\0' };
  if (loadCfg(MODE_CFG, rain_cfg)) {
    rainbow = static_cast<bool>(rain_cfg[0]);
  }

  uint8_t red_val[1] = { 255 };
  uint8_t green_val[1] = { 255 };
  uint8_t blue_val[1] = { 255 };
  loadCfg(RED_CFG, red_val);
  loadCfg(BLUE_CFG, blue_val);
  loadCfg(GREEN_CFG, green_val);

  if (LOG_ENABLED) {
    Serial.println("Loaded LED color preferences... ");
    Serial.printf("Got %s for rainbow mode\n", rainbow?"true":"false");
    Serial.printf("Got %u for red value\n", red_val[0]);
    Serial.printf("Got %u for green value\n", green_val[0]);
    Serial.printf("Got %u for blue value\n", blue_val[0]);
    Serial.println("... Converting to HTML and PWM colors...");
  }

  sprintf(saved_color, "#%02x%02x%02x%s", red_val[0], green_val[0], blue_val[0], rainbow?"+":"-");

  std::copy(saved_color, saved_color + 7, web_color);
  htmlColorPWM(web_color);
  if (LOG_ENABLED) {
    Serial.printf("set LED color: %s\n", web_color);
    Serial.printf("Red=%i, Green=%i, Blue=%i\n", red_val[0], green_val[0], blue_val[0]);
  }

  if (LOG_ENABLED) {
    Serial.printf("Saved Color string is: %s\nPreferences loaded.\n", reinterpret_cast< const char* >(saved_color));
 }
}

/*_______________________________WEBSERVER_HANDLERS______________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return 404 not found error
  if (!handleFileRead(server.uri())) { // check if the file exists in the flash memory (LittleFS),
                                       // if so, send it
    server.send(404, "text/plain", "404: File not found");
  }
}

void handleFileForbid() { // dont serve config files, etc...
  server.send(403, "text/plain", "Access Forbidden!");
}

void handleServerError() { // A last resort server error handler
  server.send(500, "text/plain", "Oops. You found a bug!");
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  if (LOG_ENABLED) {
    Serial.println("handleFileRead: " + path);
  }
  if (path.startsWith("/cfg"))
    handleFileForbid();
  if (path.endsWith("/"))
    path += "index.html";                    // If a folder is requested, send the index file
  String contentType = getContentType(path); // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) ||
      LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                    // Is there a compressed version available?
      path += ".gz";                                    // Use the compressed version
    File file = LittleFS.open(path, "r");               // Open the file
    size_t sent = server.streamFile(file, contentType); // Send it to the client
    file.close();                                       // Close the file again
    if (LOG_ENABLED) {
      Serial.print("Sent file: " + path);
      Serial.printf(" size: %i\n", sent);
    }
    return true;
  }
  if (LOG_ENABLED) {
    Serial.println("File Not Found: " + path); // If the file doesn't exist, return false
  }
  return false;
}

void handleFileUpload() { // upload a new file to the LittleFS
  File fsUploadFile;
  HTTPUpload &upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/"))
      path = "/" + path;
    if (!path.endsWith(".gz")) { // The file server always prefers a compressed version of a file
      String pathWithGz =
          path + ".gz"; // So if an uploaded file is not compressed, the existing compressed
      if (LittleFS.exists(pathWithGz)) // version of that file must be deleted (if it exists)
        LittleFS.remove(pathWithGz);
    }
    if (LOG_ENABLED) {
      Serial.println("handleFileUpload Name: " + path);
    }
    fsUploadFile = LittleFS.open(path, "w"); // Open the file for writing in LittleFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {     // If the file was successfully created
      fsUploadFile.close(); // Close the file again
      if (LOG_ENABLED) {
        Serial.printf("handleFileUpload Size: %i\n", upload.totalSize);
      }
      server.sendHeader("Location", "/success.html"); // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/html",
                  "<html><body><h1>500: couldn't create file.</h1><p><a "
                  "href='/'>Return</a></p></body></html>");
    }
  }
}

/*_________________________________WEBSOCKET_HANDLERS____________________________________________*/

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
  case WStype_DISCONNECTED: // if the websocket is disconnected
    if (LOG_ENABLED) {
      Serial.printf("WebSocket [%u] Disconnected!\n", num);
    }
    break;
  case WStype_CONNECTED: { // if a new websocket connection is established
    if (LOG_ENABLED) {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("WebSocket [%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3],
                    payload);
    }
    webSocket.broadcastTXT(web_color);
    batteryCheck();
    if (rainbow == true) {
      webSocket.broadcastTXT("R");
    }
  } break;
  case WStype_TEXT: // if new text data is received
    if (LOG_ENABLED) {
      Serial.printf("[%u] get Text: %s\n", num, payload);
    }
    if (payload[0] == '#') { // we get RGB data
      // keep track of the client color.
      std::copy(payload, payload + 7, web_color);
      htmlColorPWM(web_color);
    } else if (*payload == 'R') { // the browser sends an R when the rainbow effect is enabled
      rainbow = true;
      if (LOG_ENABLED) {
        Serial.println("Client activated rainbow mode.");
      }
      webSocket.broadcastTXT("R");
    } else if (*payload == 'N') { // the browser sends an N when the rainbow effect is disabled
      rainbow = false;
      if (LOG_ENABLED) {
        Serial.println("Client deactivated rainbow mode.");
      }
      webSocket.broadcastTXT("N");
      htmlColorPWM(web_color);
      webSocket.broadcastTXT(web_color);
    } else if (payload[0] == 'S') { // the browser sends a S to request saving color settings.
      if (LOG_ENABLED) {
        Serial.println("Client requested save color settings.");
      }
      saveColor(payload);
    } else if (*payload == 'C') {
      if (LOG_ENABLED) {
        Serial.printf("Client requested color settings, sending %s\n", web_color);
      }
      webSocket.broadcastTXT(web_color);
      if (rainbow == true) {
        if (LOG_ENABLED) {
          Serial.println("Rainbow on.");
        }
        webSocket.broadcastTXT("R");
      } else {
        if (LOG_ENABLED) {
          Serial.println("Rainbow off.");
        }
        webSocket.broadcastTXT("N");
      }
    }
    break;
  case WStype_PING:
    if (LOG_ENABLED) {
      Serial.println("Received ping, sending pong");
    }
    webSocket.broadcastTXT("pong");
    break;
  case WStype_PONG:
    if (LOG_ENABLED) {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("Received pong from %d.%d.%d.%d\n",  ip[0], ip[1], ip[2], ip[3]);
    }
    break;
  case WStype_ERROR:
    if (LOG_ENABLED) {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("Websocket error accepting payload from %d.%d.%d.%d payload: %s, size %i\n", ip[0], ip[1],
                    ip[2], ip[3], payload, length);
    }
    break;
  case WStype_BIN:
    if (LOG_ENABLED) {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("Ignoring binary Websocket payload from %d.%d.%d.%d payload: %s, size %i\n", ip[0], ip[1],
                    ip[2], ip[3], payload, length);
    }
    webSocket.broadcastTXT("binary data rejected");
    break;
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    if (LOG_ENABLED) {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("Ignoring message fragment from %d.%d.%d.%d\n",  ip[0], ip[1], ip[2], ip[3]);
    }
    webSocket.broadcastTXT("Fragmented messages unsupported");
    break; 
  }
}

/*_________________________UTILITY_FUNCTIONS_____________________________________________________*/

void htmlColorPWM(char *setcolor) {                 //  Convert HTML color and set PWM
  std::copy(setcolor, setcolor + 7, web_color); // keep client color in sync.
  // Split RGB HEX String into individual color values
  char redX[5] = {0};
  char grnX[5] = {0};
  char bluX[5] = {0};
  redX[0] = grnX[0] = bluX[0] = '0';
  redX[1] = grnX[1] = bluX[1] = 'X';
  redX[2] = setcolor[1];
  redX[3] = setcolor[2];
  grnX[2] = setcolor[3];
  grnX[3] = setcolor[4];
  bluX[2] = setcolor[5];
  bluX[3] = setcolor[6];
  int r = HTMLtoAnalog(redX);
  int g = HTMLtoAnalog(grnX);
  int b = HTMLtoAnalog(bluX);
  setLed(LED_PIN_RED, r); // write it to the LED output pins
  setLed(LED_PIN_GREEN, g);
  setLed(LED_PIN_BLUE, b);
  // Update our web_color to keep UI in sync.
  webSocket.broadcastTXT(web_color);
}

int HTMLtoAnalog(char *WebColor) {
  // Convert HEX String to integer
  int ColorNum = strtol(WebColor, NULL, 16);
  // convert 4-bit web (0-255) to 10-bit analog (0-1023) range. without floats the rounding errors
  // cause large errors with small numbers.
  float tenBit = ColorNum * 4.012f;
  // convert linear (0..512..1023) to logarithmic (0..256..1023) scale.
  // This is just an approximation to compensate for the way that LEDs and eyes work.
  float TrueColor = sq(tenBit) / LED_MAX;
  int pwmVal = round(TrueColor);
  return pwmVal;
}

void saveColor(const uint8_t *html_color) {
  // Split RGB HEX String into individual color values
  char redX[5] = {'\0'};
  char grnX[5] = {'\0'};
  char bluX[5] = {'\0'};
  redX[0] = grnX[0] = bluX[0] = {'0'};
  redX[1] = grnX[1] = bluX[1] = {'x'};
  // redX[5] = grnX[5] = bluX[5] = '\0';
  redX[2] = html_color[2];
  redX[3] = html_color[3];
  grnX[2] = html_color[4];
  grnX[3] = html_color[5];
  bluX[2] = html_color[6];
  bluX[3] = html_color[7];
  // Convert HEX String to integer
  int red_val = strtol(redX, NULL, 16);
  int green_val = strtol(grnX, NULL, 16);
  int blue_val = strtol(bluX, NULL, 16);
  if (LOG_ENABLED) {
    Serial.printf("Converted hex red %s to value %i\n", redX, red_val);
    Serial.printf("Converted hex green %s to value %i\n", grnX, green_val);
    Serial.printf("Converted hex blue %s to value %i\n", bluX, blue_val);
  }
  uint8_t redPref[1] = {'\0'};
  uint8_t greenPref[1] = {'\0'};
  uint8_t bluePref[1] = {'\0'};
  redPref[0] = red_val;
  greenPref[0] = green_val;
  bluePref[0] = blue_val;

  saveCfg(MODE_CFG, reinterpret_cast<uint8_t*>(&rainbow));
  saveCfg(RED_CFG, redPref);
  saveCfg(GREEN_CFG, greenPref);
  saveCfg(BLUE_CFG, bluePref);
  webSocket.broadcastTXT("Sy");
  webSocket.broadcastTXT(web_color);
}

void batteryCheck() {
  float Batt;
  Batt = ESP.getVcc() / 1000.0f;
  char VCC[7] = {'\0'};
  char Volts[8] = {'\0'};
  dtostrf(Batt, 6, 4, VCC);
  strcat(Volts, "V");
  strcat(Volts, VCC);
  // strcat(Volts, "\0");
  webSocket.broadcastTXT(Volts);
  // REAL voltage in the battery is 1/2 volt higher, before the diode voltage drop, so this value
  // does not represent over-drain.
  while (Batt <= 2.660f) {
    int fade = 0;
    setLed(LED_PIN_RED, fade);
    setLed(LED_PIN_GREEN, LED_OFF);
    setLed(LED_PIN_BLUE, LED_OFF);
    for (int i = 0; i <= LED_MAX; i++) {
      setLed(LED_PIN_RED, i);
    }
    for (int i = LED_MAX; i <= 128; i--) {
      setLed(LED_PIN_RED, i);
    }
  }
}

void rainbowWeb(int Ar, int Ag, int Ab) {
  if (LOG_ENABLED) {
    Serial.println("Got PWM values :");
    Serial.println(Ar);
    Serial.println(Ag);
    Serial.println(Ab);
  }

  // GPIO pwm to 8-bit web color correction
  float red_val = sqrt((Ar * LED_MAX)) / 4.011;
  float green_val = sqrt((Ag * LED_MAX)) / 4.011;
  float blue_val = sqrt((Ab * LED_MAX)) / 4.011;
  int Red = round(red_val);
  int Green = round(green_val);
  int Blue = round(blue_val);
  if (LOG_ENABLED) {
    Serial.println("Converted 8-bit web colors are:");
    Serial.println(Red);
    Serial.println(Green);
    Serial.println(Blue);
  }

  // encode to html color
  char webRGB[8];
  char redStr[3];
  char grnStr[3];
  char bluStr[3];
  itoa(Red, redStr, 16);
  itoa(Green, grnStr, 16);
  itoa(Blue, bluStr, 16);
  strcpy(webRGB, "#");
  if (Red <= 15) {
    strcat(webRGB, "0");
  } // correct #1FF6 to #01FF06 for example...
  strcat(webRGB, redStr);
  if (Green <= 15) {
    strcat(webRGB, "0");
  }
  strcat(webRGB, grnStr);
  if (Blue <= 15) {
    strcat(webRGB, "0");
  }
  strcat(webRGB, bluStr);
  strcat(webRGB, "\0");
  std::copy(webRGB, webRGB + 7, rainbowState);
  if (LOG_ENABLED) {
    Serial.print("Converted to HTML color string: ");
    Serial.println(rainbowState);
  }
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0f) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0f / 1024.0f) + "MB";
  } else {
    return String("ERROR");
  }
}

String getContentType(
    String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".svg"))
    return "image/svg+xml";
  else if (filename.endsWith(".bmp"))
    return "image/bmp";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".jpeg"))
    return "image/jpeg";
  else if (filename.endsWith(".json"))
    return "application/json";
  return "text/plain";
}

// Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
void setHue(int hue) { 
  hue %= 360;  // hue is an angle between 0 and 359°
  float radH = hue * 3.142 / 180; // Convert degrees to radians
  float rf = 0.0f;
  float gf = 0.0f;
  float bf = 0.0f;

  if (hue >= 0 && hue < 120) { // Convert from HSI color space to RGB
    rf = cos(radH * 3 / 4);
    gf = sin(radH * 3 / 4);
    bf = 0;
  } else if (hue >= 120 && hue < 240) {
    radH -= 2.09439f;
    gf = cos(radH * 3 / 4);
    bf = sin(radH * 3 / 4);
    rf = 0;
  } else if (hue >= 240 && hue < 360) {
    radH -= 4.188787f;
    bf = cos(radH * 3 / 4);
    rf = sin(radH * 3 / 4);
    gf = 0;
  }
  int r = round(rf * rf * LED_MAX);
  int g = round(gf * gf * LED_MAX);
  int b = round(bf * bf * LED_MAX);
  rainbowWeb(r, g, b);
  setLed(LED_PIN_RED, r); // Write the right color to the LED output pins
  setLed(LED_PIN_GREEN, g);
  setLed(LED_PIN_BLUE, b);
  webSocket.broadcastTXT(rainbowState);
}

void setLed(int Pin, int Value) {
  if (COMMON_ANODE) {
    analogWrite(Pin, LED_MAX - Value);    
  } else {
    analogWrite(Pin, Value);
  }
}

void fatal_error(const char *error_msg) {
  if (!LOG_ENABLED) {
    UNUSED(error_msg);
  } else {
    Serial.printf("%s\n", error_msg);
  }
  setLed(LED_PIN_GREEN, LED_OFF);
  setLed(LED_PIN_BLUE, LED_OFF);
  setLed(LED_PIN_RED, LED_MAX);
  delay(33);
  setLed(LED_PIN_RED, 768);
  delay(100);
  setLed(LED_PIN_RED, 256);
  delay(10000);
  ESP.deepSleep(0);
}

void connecting_indicator() {
  setLed(LED_PIN_RED, LED_OFF);
  setLed(LED_PIN_GREEN, LED_OFF);
  setLed(LED_PIN_BLUE, LED_OFF);
  delay(33);
  setLed(LED_PIN_BLUE, 255);
  delay(100);
  setLed(LED_PIN_BLUE, LED_MAX);
  delay(200);
}