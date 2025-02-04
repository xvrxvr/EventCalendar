#include "common.h"

#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "tftp/include/tftp_ota_server.h"

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();

    lcd.gotoxy(LCD_AP_ROW);
    lcd.print("SSID: " MASTER_WIFI_SSID "\nPSWD: " MASTER_WIFI_PASSWD "\nIP  : 192.168.4.1");

    TftpOtaServer srv;
    if (srv.start())
    {
        lcd.error("Can't start TFTP server.\nRestart please.");
        for(;;);
    }
    lcd.status("Waiting for update");
    for(;;) srv.run();
}


void LCD::print(const char* msg) // \n also handled
{
    while(msg && msg_y < 15)
    {
        int x = msg_x*16;
        char* endp = strchr(msg, '\n');
        int len = endp ? endp-msg : strlen(msg);
        if (msg_x + len <= 25) // Size is 2
        {
            text2(msg, x, msg_y*32, len);
            msg_x += len;
        }
        else // size is 1
        {
            if (msg_x*2 + len > 50) len = 50-msg_x*2;
            text(msg, x, msg_y*32, len);
            msg_x += (len+1) >> 1;
        }
        if (msg_x < 25) WRect(msg_x*16, msg_y*32, (25-msg_x)*16, 32, 0);
        if (!endp) break;
        msg = endp+1;
        msg_x=0; 
        ++msg_y;
    }
}

// Progress indicator
// 400x48, block 4x4 (3x3 + 1 px marging) => 100x12
// PI row - 11*16 = 176 (44 block row). Total - 100x16 = 1600. 
// 4M download => 2*1310 bytes/block
#define BYTES_PER_BLOCK (4*1024*1024/(100*(240-LCD_MSG_PI*32)/4)+1)

void LCD::pi_inc_loaded(size_t inc)
{
    int cur_block = loaded / BYTES_PER_BLOCK;
    loaded += inc;
    int next_block = (loaded + BYTES_PER_BLOCK - 1) / BYTES_PER_BLOCK;

    for(int i=cur_block; i<next_block; ++i)
    {
        int col = i % 100;
        int row = i / 100;
        WRect(col*4, row*4+ LCD_MSG_PI*32, 3, 3, rgb(0, 0xFF, 0));
    }
}

void reboot()
{
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 5000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
        .trigger_panic = true
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    esp_task_wdt_add(NULL);
}
