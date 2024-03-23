/**
 * Copyright 2024 Winford (Uncle Grumpy) <winford@object.stream>
 * 
 * Preferences library for ESP8266 with EEPROM storage
 * 
 * SPDX-License-Identifier: MIT
 */

/**
 * @file PrefsLib.h
 * @brief Functions for storing and retrieving preference setting in EEPROM
 * 
 * @details This header provides preference storage and retrieval functions using EEPROM for saving
 * preferences permanently.
 */

#ifndef _PREFSLIB_H_
#define _PREFSLIB_H_

#include <Arduino.h>
#include <EEPROM.h>

#include <inttypes.h>
#include <stdint.h>

#include "Config.h"

// This can safely grow, but not shrink (without removing STAs).
// Minimum size to support 1 STA config is 312 (see registers below)
#define EEPROM_SIZE 512

#define NEXT_CFG 3 // OK bytes length 2 + 1 for next preference address
#define MAX_CHAR 33

/* EEPROM Registers where our configs are stored. */
#define MODE_CFG 0 // [0]
#define RED_CFG (MODE_CFG + NEXT_CFG) // [3]
#define GREEN_CFG (RED_CFG + NEXT_CFG) // [6]
#define BLUE_CFG (GREEN_CFG + NEXT_CFG) // [9]
#define LED_INVERT_CFG (BLUE_CFG + NEXT_CFG) // [12]
#define TX_POW_CFG (LED_INVERT_CFG + NEXT_CFG) // [15]
#define AP_ENABLED_CFG (TX_POW_CFG + NEXT_CFG) // [18]
#define STA_ENABLED_CFG (AP_ENABLED_CFG + NEXT_CFG) // [21]
// [24]
/**
 * I am reserving some more space for preferences that store a bool or int
 * so that adding more will not end up changing the beginning of the longer
 * settings that require char, and a fixed point will be known for `loadCfg()`
 * to use the MAX_CHAR length instead of the short setting size of 1 block.
*/ 
// Last block for short settings (int, bool) begins at 123 ends 126 + 1 block buffer
#define SHORT_CONFIG_END 127
#define HOSTNAME_CFG (SHORT_CONFIG_END + 1) // [128]
#define ADMIN_PASSWORD_CFG (HOSTNAME_CFG + MAX_CHAR + NEXT_CFG) // [164] 128 + 33 + 3
#define AP_SSID_CFG (ADMIN_PASSWORD_CFG + MAX_CHAR + NEXT_CFG) // [201] 165 + 33 + 3
#define AP_PSK_CFG (AP_SSID_CFG + MAX_CHAR + NEXT_CFG) // [238] 202 + 33 + 3
#define STA_SSID_0_CFG (AP_PSK_CFG + MAX_CHAR + NEXT_CFG) // [275] 239 + 33 + 3
#define STA_PSK_0_CFG (STA_SSID_0_CFG + MAX_CHAR + NEXT_CFG) // [312] 276 + 33 + 3
#define STA_SSID_1_CFG (STA_PSK_0_CFG + MAX_CHAR + NEXT_CFG) // [349] 313 + 33 + 3
#define STA_PSK_1_CFG (STA_SSID_1_CFG + MAX_CHAR + NEXT_CFG) // [386] 350 + 33 + 3
#define STA_SSID_2_CFG (STA_PSK_1_CFG + MAX_CHAR + NEXT_CFG) // [423] 387 + 33 + 3
#define STA_PSK_2_CFG (STA_SSID_2_CFG + MAX_CHAR + NEXT_CFG) // [460] 424 + 33 + 3
// [501] 461 + 33 + 3


bool loadCfg(int pref_address, uint8_t* setting) {
    uint8_t cfg_len = 0;
    if (pref_address < SHORT_CONFIG_END) {
        cfg_len = 1;
    } else {
        cfg_len = MAX_CHAR;
    }

    EEPROM.begin(EEPROM_SIZE);
    uint8_t check[2];
    uint8_t valid[2] = {'O','k'};
    for(uint8_t i=0;i<2;i++) {
        check[i] = EEPROM.read(pref_address + cfg_len + i);
    }
    if ((check[0] != valid[0]) || (check[1] != valid[1])) {
        if (LOG_ENABLED) {
            Serial.printf("No valid preference stored at address %i\n", pref_address);
        }
        return false;
    }

    for(uint8_t i=0;i<cfg_len;i++) {
        setting[i] = EEPROM.read(pref_address + i);
        if (setting[i] == '\0') {
            break;
        }
    }
    EEPROM.end();
    if (LOG_ENABLED) {
            if (cfg_len > 1) {
                char *char_val = reinterpret_cast< char* >(setting);
                Serial.printf("Loaded setting address: %i, value=%s\n", pref_address, char_val);
            } else {
                int char_val = setting[0];
                Serial.printf("Loaded setting address: %i, value=%i\n", pref_address, char_val);
            }
    }
    return true;
}

bool saveCfg(int pref_address, uint8_t* setting) {
    int cfg_len = 0;
    char *char_val = reinterpret_cast< char* >(setting);
    if (pref_address < SHORT_CONFIG_END) {
        cfg_len = 1;
    } else {
        cfg_len = strlen(char_val) + 1;
    }

    EEPROM.begin(EEPROM_SIZE);
    if (cfg_len == 0) {
        // wipe stored config and invalidate
        for(uint8_t i=0;i<MAX_CHAR+2;i++) {
            EEPROM.write(pref_address + i, '\0');
        }
        Serial.printf("Erased preference stored at address %i\n", pref_address);
        return true;
    }

    for(uint8_t i=0;i<cfg_len;i++) {
        if (EEPROM.read(pref_address + i) != setting[i]) {
            EEPROM.write(pref_address + i, setting[i]);
        }
    }
    if (cfg_len < MAX_CHAR) {
        for(uint8_t i=cfg_len;i<MAX_CHAR;i++) {
            EEPROM.write(pref_address + i, '\0'); 
        }
    }
    uint8_t valid[2] = {'O','k'};
    for(int i=0;i<2;i++) {
        EEPROM.write(pref_address + cfg_len + i, valid[i]);
    }
    EEPROM.commit();
    EEPROM.end();
    if (LOG_ENABLED) {
        if (cfg_len > 1) {
            Serial.printf("Saved setting address: %i, value=%s\n", pref_address, char_val);
        } else {
            int int_val = setting[0];
            Serial.printf("Loaded setting address: %i, value=%i\n", pref_address, int_val);
        }
        
    }
    return true;
}

/** Exports **/

/**
 *  @brief Load a configuration setting from EEPROM
 *  @param pref_address EEPROM address of stored setting
 *  @param setting pointer to store result
 *  @details Preference parameters are stored as uint8_t in EEPROM.  The data stored should be
 *  recast after retrieval, if necessary.
 *  @return bool true on success, or false
 */
bool loadCfg(int setting_address, uint8_t *setting);

/**
 * @brief Store a preference setting to EEPROM
 * @param pref_address EEPROM address to store setting
 * @param setting pointer to location to preference value
 * @details Preference parameters are stored as uint8_t in EEPROM.  The data stored should be
 * recast before saving, if necessary.
 * @return bool true on success or false
 */
bool saveCfg(int setting_address,  uint8_t* setting);

#endif