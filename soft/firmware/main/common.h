#pragma once

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "estring.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <vector>
#include <utility>

#include "prnbuf.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_random.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <mdns.h>
#include <nvs_flash.h>

#include "protocol_examples_utils.h"

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <driver/uart.h>

#include <lwip/apps/netbiosns.h>
#include <lwip/inet.h>

#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_oneshot.h>

#include <soc/gpio_struct.h>
#include <soc/spi_struct.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvolatile"
#include <hal/spi_ll.h>
#pragma GCC diagnostic pop

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
    SC_WEBPing_Time  = 500,  // Time (in MS) between WEB Pings
    SC_HW_INPUT_AUTO_OFF = 1000, // Time (in MS) to automatically passivating HW Input
    SC_TouchTrackInterval = 5, // Time between Touch Track samples (in ticks)
    SC_TouchTrackDeadZone = 5, // Minimal distance in Touch Track to report move
    SC_DebounceTime = 2,    // Debounce time (in ticks)
    SC_TurnoffDelay = 10*60,  // Delay before automatically turn off LCD (in seconds)
    SC_MaxFPScope = 99,     // Maximum FP match scope that not considered as duplication
    SC_FPScope100 = 100     // What value of Scope equeal to 100% ?
};

#define FINGERPRINT_SENSOR_NORMAL_COLOR ALC_Breathing, ALC_Blue, 1
#define FINGERPRINT_SENSOR_HIDDEN       ALC_Off,       ALC_Red
#define FINGERPRINT_SENSOR_OOB_COLOR    ALC_Breathing, ALC_Red, 1

void utf8_to_dos(char*);

struct U {
    char b[4];
};

U dos_to_utf8(char sym);

inline TickType_t s2ticks(uint32_t time) {return time * 1000 / portTICK_PERIOD_MS;}
inline TickType_t ms2ticks(uint32_t time) {return time && time < portTICK_PERIOD_MS ? 1 : time / portTICK_PERIOD_MS;}

// Convert timestamp to ticks
inline TickType_t hit_to_ticks(time_t tm) 
{
    return s2ticks(tm - time(NULL));
}

class Debouncer {
    uint8_t shift_reg = 0;
public:
    bool stable() const {return shift_reg == 0x55 || shift_reg == 0xAA;}    
    bool value() const {return shift_reg == 0x55;}
    void operator <<(bool sw) {shift_reg <<= 2; shift_reg |= sw ? 1 : 2;}
    void clear() {shift_reg = 0;}
};
