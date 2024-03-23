/**
 * Copyright 2024 Winford (Uncle Grumpy) <winford@object.stream>
 *
 * User Config for Moonlight_8266
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/**
 * Change this to true to enable console logging.
 *
 * Do not enable for ESP-01, the serial pins conflict, use a different module for debugging.
*/
#define LOG_ENABLED true

/* RGB LED type used, change to false for common cathode */
#define COMMON_ANODE true

/**
 * RGB LED Pin Configuration
 *
 * PIN 0 must be left floating for ADC battery measurements (necessary for low batt warning)
 * to work!
 *
 * Recommended starting resistor values to place between each pin and LED leg are
 * commented after each pin/color. See the README for advice on tuning the resistor values for
 * color correction. If white is not a true white, but tinted or off-white, resistors should be
 * adjusted.
*/
#define LED_PIN_RED 2   // 100r resistor
#define LED_PIN_GREEN 1 // 470r resistor
#define LED_PIN_BLUE 3  // 220r resistor

/* A hostname for the DNS/DHCP and OTA services */
const char *hostName = "moon";

/* Start a wireless access point by default */
#define START_AP_DEFAULT true
/* The name of the WiFi network AP that will be created */
const char *ap_ssid = "Moonlight";
/* The password required to connect to AP, leave blank to create an open network */
const char *ap_password = "";

/**
 *  Connect to a wireless network by default
 *
 * This is not the recommended way, storing credentials in the config file is insecure.
 *
 * TODO: Up to 3 wireless networks can be added in the /admin.html control panel, so connect
 *  to the default access point and configure STA mode networks using the admin panel.
 * Credential will be saved unencrypted in EEPROM, this area of flash should be completely
 * unreachable by the HTTP server, which reads its content from LittleFS, and the passwords
 * wont be sitting around on your hard drive unencrypted.
*/
#define START_STA_DEFAULT false
/* The name of the WiFi network to join */
const char *sta_ssid = "";
/* The password required to connect to AP, leave blank to create an open network */
const char *sta_password = "";

/**
 * Admin password. Used for used for OTA and web admin network interface. Change to match the md5
 * hash of the password you want to use. This can also be changed in the admin panel, so it's best
 * to keep the default password "admin" here.
 *
 * TODO: remove this config setting and force creating a new password at the first login to the
 * admin page.
*/
const char *DefaultPassword = "21232f297a57a5a743894a0e4a801fc3"; // Default: admin

/**
 * Default WiFi TX power
 *
 * Value 0-20.5
 * PrefsLib only stores integer values in EEPROM, floats require more space, and would complicate
 * the code significantly. If you need a specific floating point value for the TX power set it
 * here. Note if a value of 21 is selected in the web interface the maximum value of 20.5 will be
 * used, all other integer values are converted to floats when the value is set in hardware.
*/
#define TX_POW 1

#endif
