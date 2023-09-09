#include "hadrware.h"

extern "C" void app_main(void)
{
    hw_init();
//    lcd.DRect(0, 0, 200, 100, 0xFFFF);
    lcd.text("Hello!", 100, 100);
    for(;;);
}