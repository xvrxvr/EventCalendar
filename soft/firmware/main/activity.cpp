#include "activity.h"
#include "input_hw.h"

static TouchInput touch_input("Input-Touch");
static FGInput fg_input("Input-FG");

Activity::Activity(int /* bitset of ActionType and ActionFlags*/ actions, int /* bitset of ActionType*/ can_be_borrowed_actions)
{

}

Activity::~Activity() {}

void Activity::setup_alarm_action(uint32_t time_to_hit)
{
}

void Activity::setup_timeout(uint32_t timeout)
{
}

void Activity::setup_web_ping_type(const char* tag)
{

}

void Activity::set_special_color_feedback_code(int)
{

}

// You SHOULD call this method if AF_CanFail was specified in Activity::Activity(). Otherwise get_action will fail with abort()
// Method returns true if this Activity successfully lock all required Actions.
bool Activity::is_ok() const
{
    return true;
}

Action Activity::get_action() //Return input action. Blocks until action will be available.
{
    return {};
}

void Activity::start()
{
    touch_input.start();
    fg_input.start();
}

LCDAccess::LCDAccess(Activity* a)
{
    assert(a || !touch_input.is_started());
    touch_input.suspend();
}

LCDAccess::~LCDAccess()
{
    touch_input.resume();
}

FPAccess::FPAccess(Activity* a)
{
    assert(a || !fg_input.is_started());
    fg_input.suspend();
}

FPAccess::~FPAccess()
{
    fg_input.resume();
}
