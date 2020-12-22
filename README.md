# Moonlight_8266
RGB Color Picker web server with ESP8266-01 (ESP-01)
-
#### Wiring Circuit  :: Resistor values for Moon Lamp.
  
0=Red (GPIO0) D3   :: 100R

2=Green (GPIO2) D4  :: 480R

3=Blue (GPIO3) Rx   :: 220R

Your GRB LED will likely be differnt so adjust the resistor values untill you get a true white.

Creates a captive WiFi network "Moonlight." Optionally multiple other network can be tried to connect to, just
adjust the code. Once you are on the same network use any browser to visit http://moon.local to control the color.
All of the webpages are self contained with no references to external resorces so it will work anywhere.  
Web pages are stored on LittleFS and can be updated at http://moon.local/edit.html
