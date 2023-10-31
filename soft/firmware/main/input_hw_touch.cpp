#include "common.h"
#include "input_hw_touch.h"
#include "setup_data.h"
#include "activity.h"

void TouchInput::enable()
{
    TouchConfig::raw_touch_config(TT_Sence);
    ei();
}

void TouchInput::disable()
{
    di();
    TouchConfig::raw_touch_config(TT_LCD);
}

void TouchInput::process_input_signal(bool is_autorepeat)
{
    db << !TouchConfig::raw_touch_config(TT_Sence);
    if (!db.stable())
    {
        set_autorepeat(DS_Touch);
        in_db = true;
        // Interrupts is disabled - we will be called by autorepeater
        return;
    }
    ei();
    if (db.stable() != prev_pressed) // Press/release
    {
        prev_pressed = !prev_pressed;
        if (prev_pressed) process_press();
        else process_release();
    }
    else // Stable
    {
        if (prev_pressed && is_autorepeat) process_autorep();
        else set_autorepeat(prev_pressed && (touch_cmd & AT_TouchTrack) ? SC_TouchTrackInterval : 0);
    }
}

// Read Touch. Returns false if Touch was lost during read (Debouncer and other will be updated)
bool TouchInput::touch_read() 
{
    TouchConfig tc;
    if (!tc.raw_touch_read()) // Lost touch during X/Y reading
    {
        db.clear();
        set_autorepeat(DS_Touch);
        prev_pressed = false;
        in_db = true;
        return false;
    }
    last_x = touch_setup.x(tc.x, tc.y);
    last_y = touch_setup.y(tc.x, tc.y);
    return true;
}

void TouchInput::process_press()
{
    if (touch_cmd & AT_TouchDown)
    {
        if (!touch_read()) return;
        Activity::push_action(Action{.type=AT_TouchDown, .touch = {.x = last_x, .y=last_y}});
    }
    if (touch_cmd & AT_TouchTrack) // Setup Tracking
    {
        in_db = false;
        set_autorepeat(SC_TouchTrackInterval);
    }
    else // Tracking is not needed
    {
        set_autorepeat();
    }
}

void TouchInput::process_release()
{
    set_autorepeat();
    if (touch_cmd & AT_TouchUp)
    {
        Activity::push_action(Action{.type=AT_TouchUp});
    }
}

void TouchInput::process_autorep()
{
    int lx = last_x, ly = last_y;
    if (!(touch_cmd & AT_TouchTrack)) return;
    if (!touch_read()) return;
    int xx = lx - last_x, yy = ly - last_y;
    if (xx*xx + yy*yy >= SC_TouchTrackDeadZone*SC_TouchTrackDeadZone) // Report move
    {
        Activity::push_action(Action{.type=AT_TouchTrack, .touch = {.x = last_x, .y=last_y}});
    }
    else
    {
        last_x = lx;
        last_y = ly;
    }
}

void TouchInput::process_cmd(uint32_t cmd)
{
    cmd &= (AT_TouchDown|AT_TouchUp|AT_TouchTrack) & ~AT_TouchBit;
    if (touch_cmd == cmd) return;
    touch_cmd = cmd;
    if (!touch_cmd) 
    {
        di();
        set_autorepeat();
        return;
    }
    ei();
    if (!(touch_cmd & AT_TouchTrack) && db.stable()) set_autorepeat();
}

void TouchInput::passivate()
{
    touch_cmd = 0;
    set_autorepeat();
    di();
}
