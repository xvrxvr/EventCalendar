#include <string.h>

#include "activity.h"
#include "input_hw.h"

static constexpr int MAX_ACTIVITIES = 4;

static TouchInput touch_input("Input-Touch");
static FGInput fg_input("Input-FG");

static Activity* all_activities[MAX_ACTIVITIES];

static SemaphoreHandle_t activity_lock;

class L {
public:
    L() {xSemaphoreTake(activity_lock, portMAX_DELAY);}
    ~L() {xSemaphoreGive(activity_lock);}
};

Activity::Activity(uint32_t /* bitset of ActionType and ActionFlags*/ actions, uint32_t /* bitset of ActionType*/ can_be_borrowed_actions) : actions(actions), can_be_borrowed_actions(can_be_borrowed_actions)
{
    actions_queue = xQueueCreate(16, sizeof(Action));
    L lock;
    int empty_idx = -1;
    for(int i=MAX_ACTIVITIES; --i;)
    {
        auto a = all_activities[i];
        if (!a) {empty_idx = i; continue;}
        if (a->actions & actions & (AT_Fingerprint|AT_TouchBit)) {status_ok = false; locked_out |= 1<< i;}
        if (a->can_be_borrowed_actions & can_be_borrowed_actions) {status_ok = false; actions &= ~AF_Override; break;} // This is definitely error - Override will not work
    }
    if (!(actions & AF_Override))
    {
        assert(status_ok || (actions & AF_CanFail));
    }
    assert(empty_idx != -1);
    all_activities[empty_idx] = this;
    my_index = empty_idx;
    if (!status_ok)
    {
        if (!(actions & AF_Override)) return; // This is error case - just return
        // Otherwise - lock out all conflicting
        status_ok = true;
        for(int i=0; i<MAX_ACTIVITIES; ++i)
        {
            auto a = all_activities[i];
            if ((locked_out >> i) & 1) a->lock_out(); else
            if (a && (actions & a->can_be_borrowed_actions)) // borrow something
            {
                a->borrowed |= actions & a->can_be_borrowed_actions;
                a->push_spc_code(IA_Borrow, actions & a->can_be_borrowed_actions);
            }
        }
    }
}

void Activity::lock_out()
{
    suspended = true;
    push_spc_code(IA_Suspend);
    if (alarm_timer) xTimerStop(alarm_timer, portMAX_DELAY);
    if (watchdog_timer) xTimerStop(watchdog_timer, portMAX_DELAY);
}

void Activity::unlock_out()
{
    suspended = false;
    push_spc_code(IA_Resume);
    if (alarm_timer) setup_alarm_action(setup_alarm_time);
    if (watchdog_timer) setup_watchdog(setup_watchdog_time);
}

Activity::~Activity() 
{
    if (alarm_timer) xTimerDelete(alarm_timer, portMAX_DELAY);
    if (watchdog_timer) xTimerDelete(watchdog_timer, portMAX_DELAY);
    vQueueDelete(actions_queue);
    {
        L lock;
        all_activities[my_index] = NULL;
        if (status_ok) // Unlock (if any) and returns
        {
            for(int i=0; i<MAX_ACTIVITIES; ++i)
            {
                auto a = all_activities[i];
                if ((locked_out >> i) & 1) a->lock_out(); else
                if (a && (actions & a->can_be_borrowed_actions)) // borrow something
                {
                    a->borrowed &= ~(actions & a->can_be_borrowed_actions);
                    a->push_spc_code(IA_Return, actions & a->can_be_borrowed_actions);
                }
            }
        }
    }
}

TickType_t hit_to_ticks(uint32_t);

Activity& Activity::setup_alarm_action(uint32_t time_to_hit)
{
    check();
    setup_alarm_time = time_to_hit;
    if (!(actions & AT_Alarm)) return *this;
    if (!alarm_timer)
    {
        alarm_timer = xTimerCreate("Activity-Alarm", hit_to_ticks(time_to_hit), 0, this, &Activity::alarm_proxy);
        xTimerStart(alarm_timer, portMAX_DELAY);
    }
    else
    {
        xTimerChangePeriod(alarm_timer, hit_to_ticks(time_to_hit), portMAX_DELAY);
    }
    return *this;
}

Activity& Activity::setup_watchdog(uint32_t timeout)
{
    check();
    setup_watchdog_time = timeout;
    auto tout = setup_watchdog_time * 1000 / portTICK_PERIOD_MS;
    if (!(actions & AT_WatchDog)) return *this;
    if (!watchdog_timer)
    {
        watchdog_timer = xTimerCreate("Activity-Watchdog", tout, 0, this, &Activity::watchdog_proxy);
        xTimerStart(watchdog_timer, portMAX_DELAY);    
    }
    else
    {
        xTimerReset(watchdog_timer, portMAX_DELAY);   
    }
    return *this;
}

Action Activity::get_action() //Return input action. Blocks until action will be available.
{
    Action result;
    check();
    for(;;)
    {
        auto actions = active_actions();
        if (actions & AT_Fingerprint)
        {
            uint32_t col = 0;
            switch(actions & AT_Fingerprint2)
            {

                case AT_Fingerprint: col = FP_NORMAL_COLOR; break;
                case AT_Fingerprint0: col = FP_NO_COLOR; break;
                case AT_Fingerprint1: col = FP_OOB_COLOR; break;
                case AT_Fingerprint2: col = custom_fg_color; break;
            }
            fg_input.cmd(col);
        }
        if (actions & AT_TouchBit) fg_input.cmd(actions & (AT_TouchBit|AT_TouchDown|AT_TouchUp|AT_TouchTrack));

        xQueueReceive(actions_queue, &result, portMAX_DELAY);
        if (result.type == AT_Internal)
        {
            switch(result.internal.code)
            {
                case IA_Suspend: on_suspend(); break;
                case IA_Resume: on_resume(LCDAccess(this).access()); break;
                case IA_Borrow: on_action_borrow(ActionType(result.internal.p1)); break;
                case IA_Return: on_action_return(ActionType(result.internal.p1)); break;
            }
            continue;
        }
    }
    return result;
}

void Activity::push_action(const Action& action)
{
    L lock;
    for(int i=0; i<MAX_ACTIVITIES; ++i)
    {
        auto a = all_activities[i];
        if (!a) continue;
        if (a->active_actions() & action.type) a->push_action_local(action);
    }
}

void Activity::on_web_ping_echo(const char* tag) // Global entry - dispatched to all active WEB ping sources
{
    L lock;
    for(int i=0; i<MAX_ACTIVITIES; ++i)
    {
        auto a = all_activities[i];
        if (!a || !a->web_ping_tag || a->locked_out || strcmp(tag, a->web_ping_tag)) continue;
        a->web_ping_counter = 0;        
    }
}

void send_web_ping_to_ws(const char*);

void Activity::send_web_ping() // Internal function - called by WEB ping thread in this module.
{
    L lock;
    for(int i=0; i<MAX_ACTIVITIES; ++i)
    {
        auto a = all_activities[i];
        if (!a || !a->web_ping_tag || a->locked_out) continue;
        send_web_ping_to_ws(a->web_ping_tag);
        ++a->web_ping_counter;
        if (a->web_ping_counter > MAX_WEB_PINGS)
        {
            a->push_action_local({.type = AT_WEBEvent, .web = {.event = WE_Logout, .logout_tag = a->web_ping_tag}});
            a->web_ping_counter = 0;
        }
    }
}

static void web_ping_task(void*)
{
    for(;;)
    {
        Activity::send_web_ping();
        vTaskDelay(WEB_PING_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

void Activity::start()
{
    activity_lock = xSemaphoreCreateMutex();
    touch_input.start();
    fg_input.start();
    xTaskCreate(web_ping_task, "WEB-Ping", WEB_PING_TASK_STACK, NULL, WEB_PING_TASK_PRIO, NULL);
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