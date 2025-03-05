#include "common.h"
#include "activity.h"
#include "hadrware.h"
#include "grid_mgr.h"
#include "icons.h"
#include "setup_data.h"
#include "doors_mgr.h"

using namespace GridManager;

static const char TAG[]= "door";

static Geometry doors_geom( Rows()
    << Row()(0, 4)(-1,4)(0,4)
    << Row()(0, 3)(0,3)(0,6)
    << Row()(0, 4)(0,4)(0,4)
);

#define SPACER_CELL_X 4
#define SPACER_CELL_Y 0

#define SPACER_CELL SPACER_CELL_Y,SPACER_CELL_X

static BoxDef doors_box{
    .box_defs = {
        "2211135U0A630E03 832B,0000,7BAF,FFFF", // Background
        "2211102U0A630E03 E71C,0000,7BAF,FFFF" // Empty Cell
    },
    .cell_width = 15,
    .cell_height = 55
};

static BoxDef& bd(uint8_t options)
{
    doors_box.reserve_left = options & DGO_ResLeft ? -1 : 0;
    return doors_box;
}

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

DoorGrid::DoorGrid(DoorGridOptions options) : Grid(bd(options), doors_geom), options(options) {}

void DoorGrid::init(uint8_t active_doors_)
{    
    for(int idx=0; idx<8; ++idx)
    {
        auto rc = enc[idx];
        set_cell_id(rc.row, rc.col, idx);
    }

    set_active_doors(active_doors_, false);

    Activity::LCDAccess lcda(NULL);
    auto& lcd = lcda.access();
    set_coord(lcd, GridManager::Rect{0, 0, RES_X, RES_Y});
    lcd.set_bg(0);

    if ((options & (DGO_CloseLabelTopRight|DGO_CloseLabelMiddle)) && !(options & DGO_CloseLabel2ndTry)) draw_icon(lcd);
}

void DoorGrid::sync()
{
    detect_stage();
    setup_cell();
    update(Lcd());
}

void DoorGrid::draw_icon(LCD& lcd) 
{
    int x, y;
    if (ico_active) return;
    switch(options & (DGO_CloseLabelTopRight|DGO_CloseLabelMiddle))
    {
        case DGO_CloseLabelTopRight: x=RES_X-32; y=0; break;
        case DGO_CloseLabelMiddle: 
        {
            auto rect = greed2screen(get_cell_coord_int(SPACER_CELL));
            x=rect.x+(rect.width-32)/2; y=rect.y;
            break;
        }
        default: return;
    }
    lcd.icon32x32(x, y, close_icon, 0xFFFF);
    ico_active = true;
}

void DoorGrid::open_physical_door(int door_index)
{
    sol_hit(door_index);
    while(!time_to_reengage_sol) vTaskDelay(100); // Get a time to Solenoid task raise and update douncounter
    if ((options & DGO_2Stage) && stage <= S_Ready) // Move to 2nd stage
    {
        stage = S_WaitForReopen;
        if (options & DGO_CloseLabel2ndTry) draw_icon(Lcd());
    }
    else
    {
        stage = S_WaitForOpen;
    }
    sync();
    draw_downcount();
}

void DoorGrid::set_active_doors(uint8_t active_doors, bool do_sync)
{
    for(int idx=0; idx<8; ++idx, active_doors >>= 1)
    {
        auto rc = enc[idx];
        set_cell_box(rc.row, rc.col, active_doors&1);
    }
    if (do_sync) sync();
}

void DoorGrid::update_doors_texts(uint8_t doors)
{
    for(int idx=0; idx<8; ++idx, doors >>= 1)
    {
        if (doors&1)
        {
            auto rc = enc[idx];
            invalidate(rc.row, rc.col);
        }
    }
    update(Lcd());
}

uint32_t DoorGrid::activity_get_actions()
{
    uint32_t act = AT_WatchDog | AT_TouchDown;
    if (options & (DGO_AnyFinger|DGO_Finger)) act |= AT_Fingerprint;
    return act;
}

bool DoorGrid::is_close_icon(const Action& act)
{
    if (!ico_active) return false;
    switch(options & (DGO_CloseLabelTopRight|DGO_CloseLabelMiddle))
    {
        case DGO_CloseLabelTopRight: return ::is_close_icon(act);
        case DGO_CloseLabelMiddle: 
        {
            auto tc = get_touch(act.touch.x, act.touch.y);
            return tc.row == SPACER_CELL_Y && tc.col == SPACER_CELL_X;
        }
        default: return false;
    }
}

uint32_t DoorGrid::activity_process(Activity &activity)
{
    bool is_running = stage == S_WaitForOpen || stage == S_WaitForReopen || time_to_reengage_sol;
    activity.setup_watchdog(is_running ? 1 : SC_TurnoffDelay);
    Action act = activity.get_action();
    switch(act.type)
    {
        case AT_WatchDog:
            if (!is_running) return ODR_Timeout;
            draw_downcount();
            if (!time_to_reengage_sol) sync();
            break;
        case AT_TouchDown:
            {
                if (is_close_icon(act)) return ODR_Finished;
                int door_idx = get_touch(act.touch.x, act.touch.y).id;
                if (door_idx != -1) return is_running ? 0 : ODR_Opened | (door_idx&7);
                return ODR_Touch|act.touch.y|(act.touch.x<<23);
            }
        case AT_Fingerprint:
            {
                int fg_user = act.fp_index >> 2;
                if (!(options & DGO_AnyFinger))
                {
                    if (fg_user == -1) {ESP_LOGI(TAG, "Door::FG: Unknown finger"); break;}
                    fg_user &= 31;
                    if (fg_user == logged_in_user) // Current user touch FG - just open door
                    {
                        if (!is_running) return ODR_Opened | 0xFF;
                        break;
                    }
                }
                return ODR_Login | (fg_user & 0xFF);
            }
        default: break;
    }
    return 0;
}

// Detect Stage and fill 'stage' member
void DoorGrid::detect_stage()
{
    bool door_not_opened_yet = (stage == S_WaitForOpen || stage == S_Ready) || !(options & DGO_2Stage);
    if (time_to_reengage_sol) stage = door_not_opened_yet ? S_WaitForOpen : S_WaitForReopen;
    else stage = door_not_opened_yet ? S_Ready : S_ReadyToReopen;
}

// Setup active Door cell based on 'stage'
void DoorGrid::setup_cell()
{
    const char* s;
    switch(stage)
    {
        case S_Ready:         s = door_available; break;
        case S_ReadyToReopen: s = (options & DGO_2Stage) ? door_available_reopen : door_available; break;
        default:              s = door_wait_for_open; break;
    }
    change_box(1, s);
}

void DoorGrid::draw_downcount()
{
    auto rect = greed2screen(get_cell_coord_int(SPACER_CELL));
    if (!time_to_reengage_sol) strcpy(txt, "   "); else
    if (time_to_reengage_sol < 10) snprintf(txt, sizeof(txt), " %d ", time_to_reengage_sol);
    else snprintf(txt, sizeof(txt), "%d", time_to_reengage_sol);

    int len = strlen(txt);
    int sym_unit = options & DGO_CloseLabelMiddle ? 8 : 16;

    rect.x += (rect.width-len*sym_unit)/2;
    if (options & DGO_CloseLabelMiddle) rect.y += rect.height-sym_unit*2;
    else rect.y += (rect.height-sym_unit*2)/2;

    Activity::LCDAccess lcda(NULL);
    auto &lcd = lcda.access();
    lcd.set_fg(0xFFFF);
    lcd.set_bg(box_defs[0].bg_color);
    if (options & DGO_CloseLabelMiddle) lcd.text(txt, rect.x, rect.y);
    else lcd.text2(txt, rect.x, rect.y);
}
