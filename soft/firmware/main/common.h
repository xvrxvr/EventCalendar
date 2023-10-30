#pragma once

#include <stdint.h>
#include <string.h>

// Priorities of various tasks
enum TasksPriority {
    TP_Hardware = 5,    // Handling of HW input
    TP_WEBPing  = 1     // WEB ping task
};

// task stack sizes
enum TaskStackSize {
    TSS_WEBPing = 1024 // WEB Ping task
};

// Generic consts
enum SetupConsts {
    SC_WEBPing_Pings = 2,   // Number of missing Pings to hit threshold
    SC_WEBPing_Time  = 500  // Time (in MS) between WEB Pings
};

#define FINGERPRINT_SENSOR_NORMAL_COLOR ALC_Breathing, ALC_Blue
#define FINGERPRINT_SENSOR_HIDDEN       ALC_Off,       ALC_Red
#define FINGERPRINT_SENSOR_OOB_COLOR    ALC_Breathing, ALC_Red

void utf8_to_dos(char*);

struct U {
    char b[4];
};

U dos_to_utf8(char sym);
