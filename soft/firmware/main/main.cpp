#include "common.h"

#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "activity.h"
#include "interactive.h"
#include "challenge_list.h"

void start_http_servers();

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();
    start_http_servers();
    Activity::start();

    for(;;) {Interactive::entry(); vTaskDelay(100);}
}