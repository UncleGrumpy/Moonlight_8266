<!--
 Copyright 2020-2024 Winford (Uncle Grumpy) <winford@object.stream>
 SPDX-License-Identifier: MIT
-->

# Moonlight_8266

[![Compile Sketch](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/build.yml) [![CodeQL](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/codeql.yml/badge.svg?branch=main)](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/codeql.yml) [![Check Arduino Formatting](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/formatting.yml/badge.svg?branch=main)](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/formatting.yml) [![Check JavaScript/HTML formatting](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/web_lint.yml/badge.svg?branch=main)](https://github.com/UncleGrumpy/Moonlight_8266/actions/workflows/web_lint.yml)

RGB Color Picker web server for ESP-01 (or ESP826)

## About

This firmware is designed to create a web interface to control an RGB LED inside a 3-d printed moon lamp.  I used this
amazing design I found at https://www.thingiverse.com/thing:4102658.

## Prerequisites

This sketch is updated to build with ArduinoIDE 2.x

This sketch depends on the [WebSockets](https://github.com/Links2004/arduinoWebSockets) library, which can be installed from the ArduinoIDE _Library Manager_.
The `WebSockets` library depends on the [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) library, also available in the ArduinoIDE _Library Manager_.

To upload the web content to the ESP8266 you will need the [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload) plugin for ArduinoIDE 2.x. (For the old java based ArduinoIDE 1.x use [arduino-esp8266littlefs-plugin](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin)) The directions for that project say to put the [vsix](https://github.com/earlephilhower/arduino-littlefs-upload/releases) file in to the `~/.arduinoIDE/plugins/` (`C:\Users\<username>\.arduinoIDE\plugins\` on Windows) directory, but for it show up in the _Command Palette_ I had to put it also in the `~/.arduinoIDE/plugin-storage/` directory. So in the end I decided to place the real `arduino-littlefs-upload-1.0.0.vsix` file into `~/.arduinoIDE/plugin-storage/` and put a symlink to that file into `~/.arduinoIDE/plugins/`. I don't know the recommended way to set up plugins, but that worked for me.

There is now optional support for the [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) library for async http server.  This library requires the [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP]) library to work on ESP8266, which should be installed automatically when you install the `ESPAsyncWebServer` through the ArduinoIDE _Library Manager_.  If both of these libraries are not installed the build will default to using the stock [ESP8266WebServer](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer) library included in the [ESP8266 core board support](https://github.com/esp8266/Arduino).

## Usage

The ESP creates a captive WiFi network `Moonlight`. Use any browser connected to the same network to visit http://moon.local to
control the color. If your browser "cannot find the server" at that address for some reason the control interface can
also be reached at: `http://192.168.4.1`.

All of the web pages are self contained with no references to external resources so it will work anywhere. This also
makes the device more secure because no requests can be redirected to malicious content. Privacy is also respected for
the same reasons, Google and your ISP won't be tracking those requests if they never happen!
Web pages are stored on LittleFS and new files or updated content can be uploaded at: http://moon.local/edit.html

## Features

* Low battery waning. When the voltage level of the battery drops too low to produce accurate color the moon will turn red and fade up and down in brightness to alert the user that the battery needs to be recharged.
* Save default color settings (including Rainbow mode) to EEPROM. Even though this board does not have a real EEPROM I decided to use the flash emulated EEPROM just to keep the hardware settings separate from the web pages that are stored on the LittleFS partition of the flash.

## Building and Flashing

`Config.h` can be edited, all configurable definitions have been moved there. If you need to change the type of RGB LED used or the pins the LED is connected to, that is the place to do it. This project was originally built using a common  All options are commented. 

Currently this is the only way to set up STA mode and connect to an existing WiFI network, but the updated preferences storing subsystem already has support for storing up to three WiFi networks.  The WiFiMulti library has been used, so once a network control page and websocket handlers are added, the web interface should be used. Setting this option in Config.h may be removed after web UI configuration is added.

After you have successfully build and uploaded the sketch you still need to upload the data files used by the HTTP server to the LittleFS filesystem in flash. If you have properly installed the [arduino-littlefs-upload](https:/ github.com/earlephilhower/arduino-littlefs-upload) plugin, it will appear in the _Command Palette_ ([Ctrl] + [Shift] + [P] or [⌘] + [Shift] + [P] on MacOS) as `Upload LittleFS to Pico/ESP8266`. This plugin will automatically create the LittleFS filesystem, add the files, and upload the content to the correct flash address. 

### ArduinoIDE recommended settings

- For extended battery life use 80 MHz for the CPU Frequency.
- Recommended formatting is (FS: 192KB / OTA: ~406KB). FS should be at least 160KB.
- IwIP variant: "v2 Higher Bandwidth"
- Flash Mode: `qio`, if your module supports it. This will speed up reading the web pages from LittleFS. Otherwise `dio`. 

## Wiring Chart (ESP-01) for Moon Lamp

| ESP-01 pin | resistor value |  RGB LED pin   |
|:-----------|:--------------:|:---------------|
| GPIO2 (D3) | 100 Ohm        | Red            |
| GPIO1 (Tx) | 470 Ohm        | Green          |
| GPIO3 (Rx) | 220 Ohm        | Blue           |
| GPIO0      | Must be left floating for voltage measurements to work! | :x: |
| __________ | _______________________________________________________ | ____________________________________________ |
| 3v3        | :x:            | PWR - For common anode (default and recommended configuration) |
| GND        | :x:            | GND - For common cathode (change `COMMON_ANODE` to `false` in **Config.h**) |

For other ESP8266 boards, you may use any other pins for the LED colors, but if you intend to
use the project battery powered `pin 0` should be left floating. This is necessary to take the ADC battery
level reading, and change the lamp to red when the device needs recharging.

The ESP-01 uses 3.3 volts, if you are using a board with 5V (like Arduino) adjust your resistor
values accordingly. Your RGB LED will likely be different so adjust the resistor values until you
get a true bright white when all three are on at 100%. The easiest way to do this is just use the
RGB LED and your test resistors on a breadboard with a 3.3V power supply. Even easier than fishing
through a pile of resistors is to use 2 variable resistors and measure the final setting with a
multimeter. Either way use a 100 Ohm resistor (220r for 5V) for the red; it will always appear the
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
nearly depleted.  That was a big part of my motivation to make the moon turn red and fade in and out when
the power was nearly depleted, along with having a way to provide a warning to the user.  The other
big advantage managing my voltage this way is that by leaving GPIO 0 floating you can take direct
battery readings with the ESP-01 without needing a voltage divider or any additional pins.  I plan on
adding a battery level indicator to the web interface, but at the moment battery voltage is being
reported on the JavaScript console of the web client.

### A safer battery design

In the next hardware revision of this project I plan on using LiFePo4 batteries, which will not
require the 1N4728 diode, since the entire safe range of the battery falls in the safe voltage
range of operation as an ESP8266, or other ESP32 chips.  This will just require adding one more
configuration option to set the low voltage detect, since the voltage measured will be the actual
battery voltage (not the voltage after the drop caused by the diode).  These batteries are much
safer than Li-ion battery chemistry.

Future revisions will likely be based on a more modern chip like the ESP32-C2, C3 or ESP32-C6. The
performance VS power consumption of the newer RISC-V chips is far superior to the old 8266.

## Note on future support

If I find the spare time I will a definition to the top of the file to allow inverting the pwm signal if
a common anode RGB LED is used, but this project is quite old and may be archived in the future.  I have
plans on a much better version, that will support ESP32, and all of the common RGB LEDs, including
NeoPixels, and DotStars in addition to the "old school" 4-pin RBG LEDs used in this version.
