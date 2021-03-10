# Moonlight_8266
##### RGB Color Picker web server for ESP-01 (or ESP826)


  Creates a captive WiFi network "Moonlight." Optionally multiple other network can listed and it will attempt each one until a connection is made, just uncomment and add the network names and passwords to the lines at the top of the "SETUP_FUNCTIONS" section of the code. Once you are on the same network use any browser to visit http://moon.local to control the color. If the does not work for some reason the control interface can be reached at: http://192.168.4.1 

  All of the web pages are self contained with no references to external resources so it will work anywhere. This also make the device more secure because no requests can be redirected to malicious content.  
Web pages are stored on LittleFS and new files or updated content can be uploaded at: http://moon.local/edit.html


#### New Features:
+ Save default color settings (including Rainbow mode) to EEPROM. Even though this board does not have a real EEPROM I decided to use the flash emulated EEPROM just to keep the hardware settings separate from the web pages that are stored on the LittleFS partition of the flash.
+ The web interface is now updated by the esp over websocket instead of directly from user input. This allows the moon to display the current color even in rainbow mode.

#### ArduinoIDE recommended settings:
  For extended battery life use 80MHz for the CPU Frequency.  
  Recommended formatting is (FS: 192KB / OTA: ~406KB). FS should be at least 160KB.  
  IwIP varient: "v2 Higher Bandwidth"

#### Wiring Circuit (ESP-01) :: Resistor values for Moon Lamp.
  
- 2=Red (GPIO2) D3   :: 100R
- 0=Green (GPIO0) D4  :: 480R
- 3=Blue (GPIO3) Rx   :: 220R

  The ESP-01 uses 3.3V exclusively, if you are using a board with 5V (Arduino) adjust your resistor values accordingly. Your RGB LED will likely be different so adjust the resistor values until you get a true bright white when all three are on at 100%. The easiest way to do this is just use the RGB LED and your test resistors on a breadboard with a 3.3V power supply. Even easier than fishing through a pile of resistors is to use 2 variable resistors and measure the final setting with a multimeter. Either way use a 100 Ohm resistor (220 for 5V) for the red; it will always appear the dimmest, and find the sweet spot for the green and blue.

  For battery powered operation I use a 100K resistor for the CH_PD pull-up. This minimized the power draw on the battery. You can use a 10K or 1K, or for bread boarding just connect it directly to the same 3.3V rail as the VCC.


#### Planed features for the next release:

##### For users:
+ Shutdown timer or automatic off time. This will be a convenient way to extend the time before needing to recharge the battery.
+ An optional start time to go along with the shutdown timer.
+ An interface to be able to change the device name and update the WiFi network settings.
+ Extend battery life by lowering the WiFi trasmit power. This will also shorten the range of the signal, but that is not necessarily a bad thing.

##### For developers and hackers:
+ Remove the use of Arduino "Strings" in the server handlers section that I included from the ArduinoIDE example sketch I used as a reference. This should be more efficient and will make the code more consistent with the use of C "strings" that I am using throughout the rest of the code.
