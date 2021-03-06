#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <EEPROM.h>
#include <cmath>

IPAddress apIP(192, 168, 4, 1);
ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);    // create a web server on port 80
WebSocketsServer webSocket(81); // create a websocket server on port 81
DNSServer dnsServer;            // create an instance of the DNSServer class, called 'dnsServer'
File fsUploadFile;      // a File variable to temporarily store the received file.

const char *ssid = "Moonlight"; // The name of the Wi-Fi network that will be created
const char *password = "";   // The password required to connect to it, leave blank for an open network
const char *hostName = "moon";           // A hostname for the DNS and OTA services
const char *OTAPassword = "31f2385ba9cc65dba7ccb9aa5c5b7600";     // OTA password md5() hash

bool rainbow;           // For rainbow mode.
char savedColor[12];     // keeping track of saved color prefrences.
char webColor[8];       // current color in HTML format.
char rainbowColor[8];   // To show the correct color on the moon during rainbow mode

#define TX_POW  1 // sets wifi power (0 lowest 20.5 highest)
// specify the pins with an RGB LED connected
#define LED_RED     2           // 100r resistor
#define LED_GREEN   1           // 470r resistor
#define LED_BLUE    3           // 220r resistor
// PIN 0 must be left floating for battery mesuremnts to work!
ADC_MODE(ADC_VCC);

// EEPROM Registers where our configs are stored.
#define MODE_STO 0
#define R_STO 1
#define G_STO 2
#define B_STO 3

/*___________________________________________________SETUP__________________________________________________________*/

void setup() {
  pinMode(LED_RED, OUTPUT);    // the pins with LEDs connected are outputs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

//  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
//  delay(10);
//  Serial.println("\r\n");
  EEPROM.begin(512);

  analogWrite(LED_RED, 1023);  // Turn on moon.
  analogWrite(LED_GREEN, 1023);
  analogWrite(LED_BLUE, 1023);

  colorInit();                // Restore saved color settings.

  startWiFi();                // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

  startDNS();                // Start the DNS and mDNS responder

  startLittleFS();            // Start the FS and list all contents

  startWebSocket();           // Start a WebSocket server

  startServer();              // Start an HTTP server with a file read handler and an upload handler

  startOTA();                 // Start the OTA service

}

/*____________________________________________________LOOP__________________________________________________________*/

unsigned long prevMillis = millis();
int hue = 0;

void loop() {
  for (int i = 0; i <= 750000; i++) {           // measure vcc voltage at aprox. 60 sec intervals. varies slightly under load, but it's not critical.
    webSocket.loop();                           // constantly check for websocket events
    dnsServer.processNextRequest();             // handle dns requests
    server.handleClient();                      // handle server requests

    if (rainbow) {                              // if the rainbow effect is turned on
      if (millis() > prevMillis + 27) {
        if (++hue == 360)                       // Cycle through the color wheel in 10 seconds (increment by one degree every 27 ms)
          hue = 0;
        setHue(hue);                            // Set the RGB LED to the right color
        prevMillis = millis();
      }
    }
    ArduinoOTA.handle();                        // Check for OTA update.
  }
  batteryCheck();
}

/*_________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.setOutputPower(TX_POW);         // sets wifi power (0 lowest 20.5 highest)
  WiFi.softAP(ssid, password);             // Start the access point
//  Serial.print("Access Point \"");
//  Serial.print(ssid);
//  Serial.println("\" started\r\n");

  //wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");   // add Wi-Fi networks you want to connect to
  //wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

//  Serial.println("Connecting");
//  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
//    delay(250);
//    Serial.print('.');
//  }
//  Serial.println("\r\n");
//  if (WiFi.softAPgetStationNum() == 0) {     // If the ESP is connected to an AP
//    Serial.print("Connected to ");
//    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
//    Serial.print("IP address:\t");
//    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
//  } else {                                   // If a station is connected to the ESP SoftAP
//    Serial.print("Station connected to ESP8266 AP");
//  }
  MDNS.update();
//  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.setPasswordHash(OTAPassword);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
//    Serial.println("Start");
//    delay(250);
  });
  ArduinoOTA.onEnd([]() {
//    Serial.println("\r\nEnd");
    analogWrite(LED_RED, 0);
    analogWrite(LED_GREEN, 1023);
    analogWrite(LED_BLUE, 0);
    delay(3000);
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // Start at full brightness and fade out until complete.
    int UploadProg = 1023 - ((progress / (total / 100))*10.2);
    analogWrite(LED_RED, 0);
    analogWrite(LED_GREEN, 0);
    analogWrite(LED_BLUE, UploadProg);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    analogWrite(LED_BLUE, 0);
    analogWrite(LED_GREEN, 0);
    analogWrite(LED_RED, 1023);
//    Serial.printf("Error[%u]: ", error);
//    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    delay(1000);
    for (int m = 0; m <= 2; m++) {
      analogWrite(LED_RED, 768);
      delay(133);
      analogWrite(LED_RED, 256);
      delay(200);
    }
    htmlPWM(webColor);
  });
  ArduinoOTA.begin();
//  Serial.println("OTA ready\r\n");
}

void startLittleFS() {    // Start LittleFS and list all contents
  LittleFS.begin();       // Start the File System
//  Serial.println("LittleFS started. Contents:");
//  {
//    Dir dir = LittleFS.openDir("/");
//    while (dir.next()) {               // List the file system contents
//      String fileName = dir.fileName();
//      size_t fileSize = dir.fileSize();
//      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str() );
//    }
//    Serial.printf("\n");
//  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                  // start the websocket server
  webSocket.onEvent(webSocketEvent);  // if there's an incomming websocket message, go to function 'webSocketEvent'
  MDNS.addService("ws", "tcp", 81);
//  Serial.println("WebSocket server started.");
}

void startDNS() { // Start the DNS and mDNS responders
  MDNS.begin(hostName);                        // start the multicast domain name server
//  Serial.print("mDNS responder started: http://");
//  Serial.print(hostName);
//  Serial.println(".local");
  dnsServer.setTTL(300);     // defaults ot 60 second TTL.
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);  // default return code is 'DNSReplyCode::NonExistentDomain'
  dnsServer.start(53, "*", apIP);  // Start DNS server redirecting all requests to the UI Serial.println("DNS responder started on port 53.");
}

void startServer() {      // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");       // go to 'handleFileUpload'
  }, handleFileUpload);
  // On page request go to function 'handleNotFound' and check if the file exists
  server.onNotFound(handleNotFound);
  server.begin();                             // start the HTTP server
  MDNS.addService("http", "tcp", 80);
//  Serial.println("HTTP server started.");
}

void colorInit() {
//  Serial.println("Reading preferences... ");
  byte raining = EEPROM.read(MODE_STO);
//  Serial.println("Got ");
//  Serial.print(raining);
//  Serial.println(" from EEPROM 0 (Rainbow)");
  byte rD = EEPROM.read(R_STO);
//  Serial.print("Got ");
//  Serial.print(rD);
//  Serial.println(" from EEPROM 1 (Red)");
  byte gD = EEPROM.read(G_STO);
//  Serial.print("Got ");
//  Serial.print(gD);
//  Serial.println(" from EEPROM 2 (Green)");
  byte bD = EEPROM.read(B_STO);
//  Serial.print("Got ");
//  Serial.print(bD);
//  Serial.println(" from EEPROM 3 (Blue)");
  if (  rD + gD + bD < 19 ) {  // In case value is too low to light led, assume no saved prefs.
//    Serial.println("not found!");
    rD = 255;
    gD = 255;
    bD = 255;
    raining = 0;
   // write default prefs to avoid this next time.
//   Serial.println("Attemping to store factory defaults in EEPROM...");
   EEPROM.write(MODE_STO, raining);
   EEPROM.write(R_STO, rD);
   EEPROM.write(G_STO, gD);
   EEPROM.write(B_STO, bD);
   EEPROM.commit();
//   Serial.println("Defaults have been configured. This should not happen again.");
  }
//  Serial.println("Read stored values, Coverting to HTML and PWM colors...");
  char buffer[3];
  char webRGB[8];
  strcpy(webRGB, "#");
  itoa ((int)rD,buffer,16);
  if ( buffer[0] <= '9' ) { strcat(webRGB, "0"); }  // corect #0FF0 to #00FF00 for example...
  strcat(webRGB, buffer);
  itoa ((int)gD,buffer,16);
  if ( buffer[0] <= '9' ) { strcat(webRGB, "0"); }  // Hackey, I know, but it works.
  strcat(webRGB, buffer);
  itoa ((int)bD,buffer,16);
  if ( buffer[0] <= '9' ) { strcat(webRGB, "0"); }  // I'm open to suggestions...
  strcat(webRGB, buffer);
  strcat(webRGB, "\0");
//  Serial.print("updating web color to ");
//  Serial.println(webRGB);
  std::copy(webRGB, webRGB+7, webColor);
//  Serial.println(webColor);
//  Serial.println("setting LED color now.");
  htmlPWM(webColor);
//  Serial.print("Red : ");
//  Serial.println(r);
//  Serial.print("Green : ");
//  Serial.println(g);
//  Serial.print("Blue : ");
//  Serial.println(b);
  if ( raining == 0 ) {
    rainbow = false;
//    Serial.println("Rainbow mode off.");
    strcat(savedColor, webColor);
    strcat(savedColor, "-\0");
//    Serial.print("Saved Color srting is: ");
//    Serial.println(savedColor);
  } else {
    rainbow = true;
//    Serial.println("Rainbow mode active.");
    strcat(savedColor, webColor);
    strcat(savedColor, "+\0");
//    Serial.print("Saved Color srting is: ");
//    Serial.println(savedColor);
  }
//  Serial.println("Preferences loaded.");
}

/*______________________________________WEBSERVER_HANDLERS__________________________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {   // check if the file exists in the flash memory (LittleFS), if so, send it
    server.send(404, "text/plain", "404: File not found" );
  }
}

void handleFileForbid() {   // dont serve config files, etc...
  server.send(403, "text/plain", "Access Forbidden!" );
}

void handleServerError() {   // A last resort server error handler
  server.send(500, "text/plain", "Ooops. You found a bug!" );
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
//  Serial.println("handleFileRead: " + path);
  if (path.startsWith("/cfg")) handleFileForbid();
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed version
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
//    Serial.println(String("\tSent file: ") + path);
    return true;
  }
//  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload() { // upload a new file to the LittleFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (LittleFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        LittleFS.remove(pathWithGz);
    }
//    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = LittleFS.open(path, "w");            // Open the file for writing in LittleFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
//      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/html", "<html><body><h1>500: couldn't create file.</h1><p><a href='/'>Return</a></p></body></html>");
    }
  }
}

/*______________________________________WEBSOCKET_HANDLERS__________________________________________________________*/

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
//      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
//        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.broadcastTXT(webColor);
        batteryCheck();
        if ( rainbow == true ) {
          webSocket.broadcastTXT("R");
        } else {
          //webSocket.broadcastTXT("N");
        }
      }
      break;
    case WStype_TEXT:                     // if new text data is received
//      Serial.printf("[%u] get Text: %s\n", num, payload);
      if (payload[0] == '#') {            // we get RGB data
        // keep track of the client color.
        std::copy(payload, payload+7, webColor);
        htmlPWM(webColor);
    } else if (*payload == 'R') {       // the browser sends an R when the rainbow effect is enabled
        rainbow = true;
//        Serial.println("Client activated rainbow mode.");
        webSocket.broadcastTXT("R");
      } else if (*payload == 'N') {       // the browser sends an N when the rainbow effect is disabled
        rainbow = false;
//        Serial.println("Client deactivated rainbow mode.");
        webSocket.broadcastTXT("N");
        htmlPWM(webColor);
        webSocket.broadcastTXT(webColor);
      } else if (payload[0] == 'S') {       // the browser sends a S to request saving color settings.
//        Serial.println("Client requested save color settings.");
        saveColor(payload);
      } else if (*payload == 'C') {
//        Serial.print("Client requested color settings, sending ");
//        Serial.println(webColor);
        webSocket.broadcastTXT(webColor);
        if ( rainbow == true ) {
//          Serial.println("Rainbow on.");
          webSocket.broadcastTXT("R");
        } else {
//          Serial.println("Rainbow off.");
          webSocket.broadcastTXT("N");
        }
      }
      break;
  }
}

/*__________________________________________TASK_FUNCTIONS__________________________________________________________*/

void htmlPWM(char *setcolor) {  //  Convert HTML color and set PWM
  std::copy(setcolor, setcolor+7, webColor);    // keep client color in sync.
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
  analogWrite(LED_RED, r);            // write it to the LED output pins
  analogWrite(LED_GREEN, g);
  analogWrite(LED_BLUE, b);
  // Update our webColor to keep UI in sync.
  webSocket.broadcastTXT(webColor);
}

int HTMLtoAnalog (char *WebColor) {
   // Convert HEX String to integer
   int ColorNum = strtol(WebColor, NULL, 16);
   // convert 4-bit web (0-255) to 10-bit analog (0-1023) range. without floats the rounding errors cause large errors with small numbers.
   float tenBit = ColorNum * 4.012;
   // convert linear (0..512..1023) to logarithmic (0..256..1023) scale.
   // This is just an approximation to compensate for the way that LEDs and eyes work.
   float TrueColor = sq(tenBit) / 1023;
   int pwmVal = round(TrueColor);
   return TrueColor;
}

void saveColor(const uint8_t * savecolor) {
  byte rain = 0;
  // Split RGB HEX String into individual color values
  char redX[5] = {0};
  char grnX[5] = {0};
  char bluX[5] = {0};
  redX[0] = grnX[0] = bluX[0] = '0';
  redX[1] = grnX[1] = bluX[1] = 'x';
  //redX[5] = grnX[5] = bluX[5] = '\0';
  redX[2] = savecolor[2];
  redX[3] = savecolor[3];
  grnX[2] = savecolor[4];
  grnX[3] = savecolor[5];
  bluX[2] = savecolor[6];
  bluX[3] = savecolor[7];
  // Convert HEX String to integer
  int redW = strtol(redX, NULL, 16);
  int grnW = strtol(grnX, NULL, 16);
  int bluW = strtol(bluX, NULL, 16);
  unsigned char redP = (unsigned char)redW;
  unsigned char grnP = (unsigned char)grnW;
  unsigned char bluP = (unsigned char)bluW;
  if ( rainbow != true ) {
    EEPROM.write(MODE_STO, rain);
//    Serial.print("Wrote ");
//    Serial.print(rain);
//    Serial.println(" to EEPROM 0 (Rainbow state)");
    EEPROM.write(R_STO, redP);
//    Serial.print("Wrote ");
//    Serial.print(redP);
//    Serial.println(" to EEPROM 1 (Red 4-bit)");
    EEPROM.write(G_STO, grnP);
//    Serial.print("Wrote ");
//    Serial.print(grnP);
//    Serial.println(" to EEPROM 2 (Green 4-bit)");
    EEPROM.write(B_STO, bluP);
//    Serial.print("Wrote ");
//    Serial.print(bluP);
//    Serial.println(" to EEPROM 3 (Blue 4-bit)");
    if (EEPROM.commit()) {
//      Serial.println("All data stored to EEPROM.");
      webSocket.broadcastTXT("Sy");
      webSocket.broadcastTXT(webColor);
    } else {
//      Serial.println("Failed to commit data to EEPROM!");
      webSocket.broadcastTXT("S:FAILED");
    }
  } else { // Save Rainbow mode active
    rain = 1;
    EEPROM.write(MODE_STO, rain);
//    Serial.print("Wrote ");
//    Serial.print(rain);
//    Serial.println(" to EEPROM 0 (Rainbow state)");
    if (EEPROM.commit()) {
//      Serial.println("All data stored to EEPROM.");
      webSocket.broadcastTXT("Sy");
    } else {
//      Serial.println("Failed to commit data to EEPROM!");
      webSocket.broadcastTXT("S:FAILED");
    }
  }
}
void batteryCheck() {
  float Batt;
  Batt = ESP.getVcc() / 1000.0;
  char VCC[6] = {0};
  char Volts[7] = {0};
  gcvt (Batt, 4, VCC);
  strcat(Volts, "V");
  strcat(Volts, VCC);
  strcat(Volts, "\0");
  webSocket.broadcastTXT(Volts);
  while ( Batt <= 2.660) {
    int fade=0;
    analogWrite(LED_RED, fade);
    analogWrite(LED_GREEN, 0);
    analogWrite(LED_BLUE, 0);
    for (int i = 0; i <= 1023; i++) {
        analogWrite(LED_RED, i);
    }
    for (int i = 1023; i <= 128; i--) {
        analogWrite(LED_RED, i);
    }
  }
}
void rainbowWeb(int Ar, int Ag, int Ab) {
//  Serial.println("Got PWM values :");
//  Serial.println(Ar);
//  Serial.println(Ag);
//  Serial.println(Ab);
  // GPOI pwm to 8-bit web color correction
  float Rd = sqrt((Ar * 1023)) / 4.011;
  float Gd = sqrt((Ag * 1023)) / 4.011;
  float Bd = sqrt((Ab * 1023)) / 4.011;
  int Rx = round(Rd);
  int Gx = round(Gd);
  int Bx = round(Bd);
//  Serial.println("Converted 8-bit web colors are:");
//  Serial.println(Rx);
//  Serial.println(Gx);
//  Serial.println(Bx);
  // encode to htlm color
  char webRGB[8];
  char redStr[3];
  char grnStr[3];
  char bluStr[3];
  itoa(Rx,redStr,16);
  itoa(Gx,grnStr,16);
  itoa(Bx,bluStr,16);
  strcpy(webRGB, "#");
  if ( Rx <= 15 ) { strcat(webRGB, "0"); }  // corect #0FF0 to #00FF00 for example...
  strcat(webRGB, redStr);
  if ( Gx <= 15 ) { strcat(webRGB, "0"); }
  strcat(webRGB, grnStr);
  if ( Bx <= 15 ) { strcat(webRGB, "0"); }
  strcat(webRGB, bluStr);
  strcat(webRGB, "\0");
  std::copy(webRGB, webRGB+7, rainbowColor);
//  Serial.print("Converted to HTLM color string: ");
//  Serial.println(rainbowColor);
  return;
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String("ERROR");
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".bmp")) return "image/bmp";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".jpeg")) return "image/jpeg";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

void setHue(int hue) { // Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
  hue %= 360;                   // hue is an angle between 0 and 359°
  float radH = hue * 3.142 / 180; // Convert degrees to radians
  float rf, gf, bf;

  if (hue >= 0 && hue < 120) {  // Convert from HSI color space to RGB
    rf = cos(radH * 3 / 4);
    gf = sin(radH * 3 / 4);
    bf = 0;
  } else if (hue >= 120 && hue < 240) {
    radH -= 2.09439;
    gf = cos(radH * 3 / 4);
    bf = sin(radH * 3 / 4);
    rf = 0;
  } else if (hue >= 240 && hue < 360) {
    radH -= 4.188787;
    bf = cos(radH * 3 / 4);
    rf = sin(radH * 3 / 4);
    gf = 0;
  }
  int r = rf * rf * 1023;
  int g = gf * gf * 1023;
  int b = bf * bf * 1023;
  rainbowWeb(r, g, b);
  analogWrite(LED_RED, r);    // Write the right color to the LED output pins
  analogWrite(LED_GREEN, g);
  analogWrite(LED_BLUE, b);
  webSocket.broadcastTXT(rainbowColor);
}
