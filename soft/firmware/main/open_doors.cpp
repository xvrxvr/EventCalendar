#include "common.h"
#include "activity.h"
#include "setup_data.h"
#include "bg_image.h"
#include "doors_mgr.h"

uint32_t open_door(int door_index)
{
    DoorGrid doors(DGOProfile_OpenDoor);
    bg_images.draw(Activity::LCDAccess(NULL).access());
    doors.init(1<<door_index);

    for(;;)
    {
        Activity activity(doors.activity_get_actions());
        auto action = doors.activity_process(activity);

        switch(action & 0xFF00)
        {
            case ODR_Opened: 
                if ((action & 0xFF) != door_index) break;
                doors.open_physical_door(door_index);
                working_state.unload_gift(door_index);
                if (!working_state.is_guest_type()) working_state.enabled_users &= ~bit(logged_in_user);
                working_state.sync();
                break;
            case 0: break;
            default: return action;
        }
    }
}
