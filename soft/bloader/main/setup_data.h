#pragma once

// This module contains various setup data thet holds in EEPROM and RTC RAM

// EEPROM layout (EEPROM size is 4K, 128 blocks by 32 bytes)
// Block number for different data
enum EEPROMShifts {
    ES_Global,                  // GolbalSetup (reserved space up to 4 blocks)
    ES_SSID = ES_Global + 4,    // SSID
    ES_Passwd,                  // Password (2 blocks)
    ES_Touch  = ES_Passwd+2,    // TouchSetup

    ES_UsedQ,                   // Used questions (1 block per user)
    ES_UsedQNext = ES_UsedQ + 32, // Marker
    
    ES_TOP_MAX, // Marker, not a real block
    
    ES_User     = 80,   // UserSetup[32]    
    ES_UName    = 96    // UserName[32]  (char[32][32])
};
static_assert(ES_TOP_MAX < ES_User, "EEPROM overflow");

///////////////////////////////////////////////

extern uint8_t ssid[33];
extern uint8_t passwd[64];

void init_or_load_setup();
