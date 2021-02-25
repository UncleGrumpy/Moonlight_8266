# Moonlight_8266
##### RGB Color Picker web server for ESP-01 (or ESP826)


  Creates a captive WiFi network "Moonlight." Optionally multiple other network can listed and it will attempt each one until a connection is made, just adjust the code. Once you are on the same network use any browser to visit http://moon.local to control the color.

  All of the webpages are self contained with no references to external resources so it will work anywhere.  
Web pages are stored on LittleFS and can be uploaded at http://moon.local/edit.html

  
#### Wiring Circuit (ESP-01) :: Resistor values for Moon Lamp.
  
- 2=Red (GPIO2) D3   :: 100R
- 0=Green (GPIO0) D4  :: 480R
- 3=Blue (GPIO3) Rx   :: 220R

  The ESP-01 uses 3.3V exclusively, if you are using a board with 5V (Arduino) adjust your resistor values accordingly. Your RGB LED will likely be different so adjust the resistor values until you get a true bright white when all three are on at 100%. The easiest way to do this is just use the RGB LED and your test resistors on a breadboard with a 3.3V power supply. Even easier than fishing through a pile of resistors is to use 2 variable resistors and measure the final setting with a multimeter. Either way use a 100 Ohm resistor (220 for 5V) for the red; it will always appear the dimmest, and find the sweet spot for the green and blue.

  For battery powered operation I use a 100K resistor for the CH_PD pull-up. This minimized the power draw on the battery. You can use a 10K or 1K, or for bread boarding just connect it directly to the same 3.3V rail as the VCC.


*The save function does not work yet...*
