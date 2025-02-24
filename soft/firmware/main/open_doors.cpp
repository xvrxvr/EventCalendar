#include "common.h"
#include "activity.h"
#include "hadrware.h"
#include "setup_data.h"
#include "bg_image.h"
#include "grid_mgr.h"
#include "icons.h"
#include "interactive.h"

using namespace GridManager;

static const char TAG[]= "door";

static Geometry doors_geom( Rows()
    << Row()(0, 4)(-1,4)(0,4)
    << Row()(0, 3)(0,3)(0,6)
    << Row()(0, 4)(0,4)(0,4)
);

static BoxDef doors_box{
    .box_defs = {
        "2211135U0A630E03 832B,0000,7BAF,FFFF", // Background
        "2211102U0A630E03 E71C,0000,7BAF,FFFF" // Empty Cell
    },
    .cell_width = 15,
    .cell_height = 55
};

// Door in countdown state
static const char door_wait_for_open[] = "2211102U0A630E03 D6FD,0000,7BAF,0020"; 

// Door available to open
static const char door_available[] = "2211102U0A630E03 5B1F,0000,7BAF,0020";

// Door available to reopen
static const char door_available_reopen[] = "2211102U0A630E03 C552,0000,7BAF,0020";

// Recode from door index to row/column
struct RowCol {
    int row;
    int col;
};
static RowCol enc[] = {
    {.row=0, .col=0}, {.row=0, .col=8},
    {.row=1, .col=0}, {.row=1, .col=3}, {.row=1, .col=6},
    {.row=2, .col=0}, {.row=2, .col=4}, {.row=2, .col=8}
};

#define Lcd() Activity::LCDAccess(NULL).access()

class DoorGrid : public Grid {
    enum Stage {
        S_WaitForOpen,  // Initial wait for possibility to open door_available
        S_Ready,        // Ready to open door
        S_WaitForReopen, // Wait for possibility to reopen door
        S_ReadyToReopen // Door can be reopened
    } stage;

    char txt[2]; // Downcounter. Only 1 symbol possible
    bool door_not_opened_yet = true;
    const int door_index;

    // Detect Stage and fill 'stage' member
    void detect_stage();

    // Setup active Door cell based on 'stage'
    void setup_cell();

    // Activity with downcounter (S_WaitForOpen and S_WaitForReopen)
    uint32_t run_dc_activity();

    // Activity without downcounter
    uint32_t run_stable_activity();

    uint32_t result(uint32_t res)
    {
        if (!door_not_opened_yet) res |= ODR_Opened;
        return res;
    }

    void open_door(LCD&);

    void draw_icon(LCD& lcd) {lcd.icon32x32(RES_X-32, 0, close_icon, 0xFFFF);}

    // ID: -1 - spacer
    //      0 - not active cell
    //      1 - active cell (can be pressed)
    //      <countdown-counter>+2 - In countdown
public:
    DoorGrid(int door_index) : Grid(doors_box, doors_geom), door_index(door_index) {}

    virtual const char* get_text_dos(const MiniCell& cell)
    {
        if (cell.id < 2) return NULL;
        txt[0] = cell.id + ('0' - 2);
        txt[1] = 0;
        return txt;
    }

    void init()
    {
        auto rc = enc[door_index];
        set_cell_box(rc.row, rc.col, 1);
        detect_stage();
        setup_cell();
        update_scene(Lcd(), true);
    }

    uint32_t run_activity()
    {
        detect_stage();
        setup_cell();
        update(Lcd());
        if (stage == S_WaitForOpen || stage == S_WaitForReopen) return run_dc_activity();
        else return run_stable_activity();
    }

    void update_scene(LCD& lcd, bool first_entry = false) 
    {
        bg_images.draw(lcd);
        if (first_entry) set_coord(lcd, GridManager::Rect{0, 0, RES_X, RES_Y});
        else update_all(lcd);
        lcd.set_bg(0);
        if (!door_not_opened_yet) draw_icon(lcd);
    }
};

class DoorActivity : public Activity {
    DoorGrid& grid;

public:
    DoorActivity(int actions, DoorGrid& grid) : Activity(actions), grid(grid) {}

    virtual void update_scene(LCD& lcd) override {grid.update_scene(lcd);}
};

// Detect Stage and fill 'stage' member
void DoorGrid::detect_stage()
{
    if (time_to_reengage_sol) stage = door_not_opened_yet ? S_WaitForOpen : S_WaitForReopen;
    else stage = door_not_opened_yet ? S_Ready : S_ReadyToReopen;
}

// Setup active Door cell based on 'stage'
void DoorGrid::setup_cell()
{
    auto rc = enc[door_index];
    switch(stage)
    {
        case S_WaitForOpen: case S_WaitForReopen: set_cell_id(rc.row, rc.col, time_to_reengage_sol+2);  change_box(1, door_wait_for_open); break;
        case S_Ready: set_cell_id(rc.row, rc.col, 1); change_box(1, door_available); break;
        case S_ReadyToReopen: set_cell_id(rc.row, rc.col, 1);  change_box(1, door_available_reopen); break;
    }
}

// Activity with downcounter (S_WaitForOpen and S_WaitForReopen)
uint32_t DoorGrid::run_dc_activity()
{
    DoorActivity activity(AT_WatchDog | AT_TouchDown, *this);
    activity.setup_watchdog(1);

    for(;;)
    {
        Action act = activity.get_action();
        switch(act.type)
        {
            case AT_WatchDog:
                if (!time_to_reengage_sol) return 0;
                setup_cell();
                update(Lcd());
                break;
            case AT_TouchDown:
                if (is_close_icon(act)) return result(ODR_Finished);
                break;
            default: break;
        }
    }
}

// Activity without downcounter
uint32_t DoorGrid::run_stable_activity()
{
    DoorActivity activity(AT_WatchDog | AT_TouchDown | AT_Fingerprint, *this);
    activity.setup_watchdog(SC_TurnoffDelay);
    for(;;)
    {
        Action act = activity.get_action();
        switch(act.type)
        {
            case AT_WatchDog: return result(ODR_Timeout);
            case AT_TouchDown:
            {
                if (get_touch(act.touch.x, act.touch.y).id == 1)
                {
                    open_door(Lcd());
                    return 0; // Restart Activity
                }
                if (is_close_icon(act)) return result(ODR_Finished);
                break;
            }
            case AT_Fingerprint:
            {
                int fg_user = act.fp_index;
                if (fg_user == -1) {ESP_LOGI(TAG, "Door::FG: Unknown finger"); break;}
                fg_user >>= 2;
                fg_user &= 31;
                if (fg_user == logged_in_user) // Current user touch FG - just open door
                {
                    open_door(Lcd());
                    return 0;
                }
                // Someone else touch FG - test if he can login now
                if (door_not_opened_yet) break; // Do not autologoff current user if door wasn't yet opened
                UserSetup usr;
                usr.load(fg_user, NULL);
                if (auto err = Interactive::test_user_login(usr, fg_user)) 
                {
                    ESP_LOGI(TAG, "Door::FG: User %d can't login: %s", fg_user, err);
                    break;
                }
                return result(ODR_Login) | fg_user;
            }
            default: break;
        }
    }
}

void DoorGrid::open_door(LCD& lcd)
{
    sol_hit(door_index);
    while(!time_to_reengage_sol) vTaskDelay(100); // Get a time to Solenoid task raise and update douncounter
    if (door_not_opened_yet)
    {
        draw_icon(lcd);
        door_not_opened_yet = false;
        working_state.unload_gift(door_index);
        if (!working_state.is_guest_type()) working_state.enabled_users &= ~bit(logged_in_user);
        working_state.sync();
    }
}

uint32_t open_door(int door_index)
{
    DoorGrid grid(door_index);
    grid.init();
    for(;;)
    {
        if (auto res = grid.run_activity()) return res;
    }
}
