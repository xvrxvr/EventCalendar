#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();

    lcd.text("Hello!", 100, 100);
    for(;;);
}