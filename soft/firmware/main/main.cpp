#include "common.h"

#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "activity.h"
#include "interactive.h"
#include "challenge_list.h"
#include "log_control.h"

void start_http_servers();

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();
    start_http_servers();
    
    if (TouchConfig().touched()) process_initial_log_setup();

    Activity::start();

    for(;;) {Interactive::entry(); vTaskDelay(100);}
}