# Moonlight_8266

## RGB Color Picker web server for ESP-01 (or ESP826)

This firmware is designed to create a web interface to control an RGB LED inside a 3-d printed moon lamp.  I used this
amazing design I found at https://www.thingiverse.com/thing:4102658.

## Prerequisites

This application depends on the [WebSockets](https://github.com/Links2004/arduinoWebSockets) library, which can be installed from the ArdionoIDE _Library Manager_.
The `WebSockets` library depends on the [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) library, also available in the ArduinoIDE _Library Manager_.

## Usage

 The ESP creates a captive WiFi network "Moonlight." Optionally multiple other network can listed and it will attempt
each one until a connection is made, just uncomment and add the network names and passwords to the lines at the top of
the "SETUP_FUNCTIONS" section of the code. Use any browser connected to the same network to visit http://moon.local to
control the color. If your browser "cannot find the server" at that address for some reason the control interface can
also be reached at: http://192.168.4.1 when used in Access Point mode.

 All of the web pages are self contained with no references to external resources so it will work anywhere. This also
makes the device more secure because no requests can be redirected to malicious content. Privacy is also respected for
the same reasons, Google and your ISP won't be tracking those requests if they never happen!
Web pages are stored on LittleFS and new files or updated content can be uploaded at: http://moon.local/edit.html


## New Features:
+ Low battery waning. When the voltage level of the battery drops too low to produce accurate color the moon will turn
   red and fade up and down in brightness to alert the user that the battery needs to be recharged.
+ Improved web interface -- especially for small screens.
+ Save default color settings (including Rainbow mode) to EEPROM. Even though this board does not have a real EEPROM I
   decided to use the flash emulated EEPROM just to keep the hardware settings separate from the web pages that are
   stored on the LittleFS partition of the flash.
+ The web interface is now updated by the esp over web socket instead of directly from user input. This allows the moon
   to display the current color, even in rainbow mode.
+ Extended battery life by lowering the WiFi transmit power.

## ArduinoIDE recommended settings:
  For extended battery life use 80 MHz for the CPU Frequency.
  Recommended formatting is (FS: 192KB / OTA: ~406KB). FS should be at least 160KB.
  IwIP variant: "v2 Higher Bandwidth"

## Wiring Circuit (ESP-01) :: Resistor values for Moon Lamp.

- GPIO2 <= Red   (D3)  :: 100 Ohm
- GPIO1 <= Green (D4)  :: 470 Ohm
- GPIO3 <= Blue  (Rx)  :: 220 Ohm
- GPIO0 <= Must be left floating for voltage measurements to work!

  The ESP-01 uses 3.3 volts, if you are using a board with 5V (like Arduino) adjust your resistor
values accordingly. Your RGB LED will likely be different so adjust the resistor values until you
get a true bright white when all three are on at 100%. The easiest way to do this is just use the
RGB LED and your test resistors on a breadboard with a 3.3V power supply. Even easier than fishing
through a pile of resistors is to use 2 variable resistors and measure the final setting with a
multimeter. Either way use a 100 Ohm resistor (220 for 5V) for the red; it will always appear the
dimmest, and find the sweet spot for the green and blue.
  For battery powered operation I use a 100K resistor for the CH_PD pull-up. This minimized the
power drain on the battery. You can use a 10K or 1K, or for bread boarding just connect it directly
to the same 3.3V rail as the VCC.

  I used an 18650 Li-ion battery and USB charge/discharge module for power. After trying may buck 
converters I ended up using a 1N4728 diode, in line with the + voltage as a voltage "regulator"
instead.  This is ABSOLUTELY NOT foolproof. My charge regulator stops charging at 4.1 volts, many
will charge up to 4.2 volts. If you are going to try this make sure to check your data sheets, take
measurements and make adjustments if necessary.  This diode causes about a 1/2 volt drop, bringing
the output of a full battery down to 3.6 volts - the safe maximum voltage for the ESP-01. My battery
regulator cuts power when the battery drops below 3.5 volts. Again this works out perfectly because
the ESP-01 can safely operate down to 3.0 volts, which is exactly the power being supplied after
the diode drops 1/2 volt. The over discharge protection of my module cuts power before the ESP browns
out, but the LEDs will not be supplied with enough current to operate reliably when the battery is very
nearly dead.  That was a big part of my motivation to make the moon turn red and fade in and out when
the power was nearly depleted, along with having a way to provide a warning to the user.  The other
big advantage managing my voltage this way is that by leaving GPIO 0 floating you can take direct
battery readings with the ESP-01 without needing a voltage divider or any additional pins.  I plan on
adding a battery level indicator to the web interface, but at the moment battery voltage is being
reported on the JavaScript console of the web client.


## Note on future support
This project is quite old and may be archived in the future.  I have plans on a much better version,
that will support ESP32, and all of the common RGB LEDs, including NeoPixels, and DotStars in addition
to the "old school" 4-pin RBG LEDs used in this version.
