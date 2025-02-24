#include "common.h"
#include "activity.h"
#include "setup_data.h"
#include "buttons.h"
#include "bg_image.h"

enum DoorAction {
    DA_Open = 100,
    DA_GiftUnload
};

void gift_load()
{
    bg_images.peek_next_bg_image();
    bg_images.draw(Activity::LCDAccess(NULL).access());

    Buttons b(true);

    uint32_t all_users = get_eeprom_users(US_Enabled|US_Paricipated, US_Enabled|US_Paricipated);
    for(int i=0; i<32; ++i)
    {
        if (all_users & bit(i))
        {
            b.add_button(EEPROMUserName(i).dos(), i, true);
        }     
    }
    b.add_button("Открыть", DA_Open, false);
    b.add_button("Выгрузить", DA_GiftUnload, false);

    b.draw(0, 0, 300, RES_Y);

    Activity activity(AT_TouchDown);
    for(;;)
    {
        Action act = activity.get_action();
        switch(act.type)
        {
            case AT_TouchDown: 
                if (is_close_icon(act)) return;
                printf("Pressed: %d\n", b.press(act.touch.x, act.touch.y)); 
                break;
            default: break;
        }
    }
}
