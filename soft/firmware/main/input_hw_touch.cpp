#include "common.h"
#include "input_hw_touch.h"

void TouchInput::push_action(ActionType tp)
{
    if (touch_cmd & tp) Activity::push_action(Action{.type=tp, .touch = {.x = last_x, .y=last_y}});
}

void TouchInput::event_track()   // Tracking event
{
    int lx = last_x, ly = last_y;
    if (!(touch_cmd & AT_TouchTrack)) return;
    if (!touch_read()) return;
    int xx = lx - last_x, yy = ly - last_y;
    if (xx*xx + yy*yy >= SC_TouchTrackDeadZone*SC_TouchTrackDeadZone) // Report move
    {
        push_action(AT_TouchTrack);
    }
    else
    {
        last_x = lx;
        last_y = ly;
    }
}

// Read Touch. Returns false if Touch was lost during read (Debouncer and other will be updated)
bool TouchInput::touch_read() 
{
    int x, y;
    if (!TouchConfig::raw_touch_read(x, y)) // Lost touch during X/Y reading
    {
        pin_state_process(true);
        return false;
    }
    last_x = touch_setup.x(x, y);
    last_y = touch_setup.y(x, y);
    return true;
}
