#include "common.h"
#include "activity.h"
#include "setup_data.h"
#include "buttons.h"
#include "bg_image.h"
#include "doors_mgr.h"

enum DoorAction {
    DA_Open = 100,
    DA_GiftUnload
};

class GiftsDoors : public DoorGrid {
    enum Mode {
        M_Simple,
        M_Open,
        M_Unload
    } mode = M_Simple;

    uint8_t active_doors = 0;
    int user_to_load = -1;
    Prn prn;

    uint8_t get_active_doors();

public:
    GiftsDoors(int utl) : DoorGrid(DGOProfile_GiftLoad), user_to_load(utl) {}

    void init() {DoorGrid::init(get_active_doors());}

    virtual const char* get_user_text_dos(int door_index) override;

    void process_door_open(int door_index);
    void process_button_press(int btn_index);
};

uint8_t GiftsDoors::get_active_doors()
{
    if (mode == M_Open) {active_doors=0xFF; return 0xFF;}
    if (mode == M_Simple && user_to_load==-1) {active_doors=0; return 0;}
    active_doors = 0;
    for(int idx=0; idx<8; ++idx)
    {
        auto ws = working_state.load_state[idx]; // State of loading of all doors: <position-in-queue:3 bit><user-index:5 bit>. Value of FF means 'unloaded'
        if (ws == 0xFF) active_doors |= 1 << idx;
    }
    if (mode==M_Unload) active_doors = ~active_doors;
    return active_doors;
}

void GiftsDoors::process_door_open(int door_index)
{
    if (door_index > 7) return;
    if (!((active_doors >> door_index) & 1)) return;
    if (mode == M_Simple && user_to_load == -1) return;

    open_physical_door(door_index);
    switch(mode)
    {
        case M_Simple:     
            {
                int total = working_state.total_loaded_gift(user_to_load);
                working_state.load_state[door_index] = (total << 5) | user_to_load;
            }
            break;
        case M_Unload: working_state.unload_gift(door_index); break;
        default: return;
    }
    working_state.sync();
    set_active_doors(get_active_doors());
    uint8_t usr = 0;
    for(int idx=0; idx<8; ++idx)
    {
        auto ws = working_state.load_state[idx];
        if (ws != 0xFF && (ws&31)==user_to_load) usr |= 1 << idx;
    }
    update_doors_texts(usr);
}

void GiftsDoors::process_button_press(int btn_index)
{
    if (btn_index < 0) return;
    user_to_load = -1;
    switch(btn_index)
    {
        case DA_Open: mode = M_Open; break;
        case DA_GiftUnload: mode = M_Unload; break;
        default: mode = M_Simple; user_to_load = btn_index; break;
    }
    set_active_doors(get_active_doors());
}

const char* GiftsDoors::get_user_text_dos(int door_index)
{
    if (door_index < 0 || door_index >= 8) return NULL;
    auto ws = working_state.load_state[door_index];
    if (ws == 0xFF) return NULL;

    prn.strcpy(EEPROMUserName(ws&31).dos());

    if ((ws & 0xE0) || working_state.total_loaded_gift(ws & 31) > 1) // With counter
    {
        prn.cat_printf("\n(%d)", (ws >> 5)+1);
    }
    return prn.c_str();
}


#define Lcd() Activity::LCDAccess(NULL).access()

inline const char* trim(const char* name) {if (strlen(name) > 7) ((char*)name)[7]=0; return name;}

void gift_load()
{
    bg_images.peek_next_bg_image();
    bg_images.draw(Activity::LCDAccess(NULL).access());

    uint32_t all_users = get_eeprom_users(US_Enabled|US_Paricipated, US_Enabled|US_Paricipated);
    GiftsDoors doors(all_users == 0x80000000 ? 31 : -1);
    doors.init();

    Buttons b(true);

    for(int i=0; i<32; ++i)
    {
        if (all_users & bit(i))
        {
            b.add_button(trim(EEPROMUserName(i).dos()), i, true, all_users == 0x80000000);
        }     
    }
    b.add_button("Открыть", DA_Open, false);
    b.add_button("Выгр.", DA_GiftUnload, false);

//    auto rect = doors.get_reserved(false);
//    printf("rect=%d,%d,%d,%d\n", rect.x, rect.y, rect.width, rect.height);
//    b.draw(rect.x, rect.y, rect.width, rect.height);
    b.draw(0, 0, 100, 240);

    for(;;)
    {
        Activity activity(doors.activity_get_actions());
        auto action = doors.activity_process(activity);

        switch(action & 0xFF00)
        {
            case ODR_Opened: doors.process_door_open(action&0xFF); break;
            case ODR_Touch:  doors.process_button_press(b.press(action>>23, action&0xFF)); break;
            case 0: break;
            default: return;
        }
    }
}
