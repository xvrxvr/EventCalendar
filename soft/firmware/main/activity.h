#pragma once

#include <stdint.h>
#include <assert.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/semphr.h>

#include "hadrware.h"

// !!!
enum {
    FP_NORMAL_COLOR = 0x1234,
    FP_NO_COLOR = 0x3456,
    FP_OOB_COLOR = 0x3333
};

static constexpr int MAX_WEB_PINGS = 2;
static constexpr int WEB_PING_INTERVAL_MS = 500;
static constexpr int WEB_PING_TASK_STACK = 1024;
static constexpr int WEB_PING_TASK_PRIO = 1;

enum ActionType {
    AT_Internal        = 0, // Internal
    AT_Fingerprint     = 0x0001,   // fingerprint with normal color feedback (and FP bit)
    AT_Fingerprint0    = 0x0003,   // fingerprint without color feedback
    AT_Fingerprint1    = 0x0005,   // fingerprint with OOB color feedback
    AT_Fingerprint2    = 0x0007,   // fingerprint with special color feedback
    AT_TouchBit        = 0x0008,   // This event can be checked for any touch
    AT_TouchDown       = 0x0018,
    AT_TouchUp         = 0x0028,
    AT_TouchTrack      = 0x0048,
    AT_WatchDog        = 0x0080,
    AT_WEBEvent        = 0x0100,
    AT_Alarm           = 0x0200,

    ATLast
};

enum ActionFlags {
    AF_CanFail          = 0x80000000,   // Constructor can fail - not all required ActionTypes available
    AF_Override         = 0x40000000,   // Suspend current Action for duration of new one

    AFLast
};

static_assert(uint32_t(AFLast) > uint32_t(ATLast), "ActionFlags and ActionType overlaped!");


enum WebEvents {
    WE_Login,     // No params
    WE_Logout,    // logout_tag
    WE_GameStart, // No params
    WE_GameEnd,   // No params
    WE_FGDel,     // p1 - <User-index>*4 + <FG-index-in-lib>
    WE_FGEdit     // p1 - User index
};

struct Action {
    ActionType type;
    union {
        struct {
            int x, y;
        } touch;
        int fp_index;
        struct {
            WebEvents event;
            union {
                int p1;
                const char* logout_tag;
            };
        } web;
        struct {
            int code;
            int p1;
        } internal;
    };
};

class Activity {
    enum InternalAction {
        IA_Suspend,
        IA_Resume,
        IA_Borrow,
        IA_Return
    };
    bool status_ok = true; // Set to true if all Activity sources was successfully locked
    bool status_checked = false; // Set to true if is_ok() was called
    int my_index = -1; // Index of this Activity in array of Activities
    uint32_t actions; // Set of Actions belong to this Activity
    uint32_t can_be_borrowed_actions; // Set of Actions which can be borrowed
    uint32_t borrowed = 0; // Set of Actions that was really borrowed
    uint32_t locked_out = 0; // Set of indexes of other Activities that was locked out by this one (by AF_Override option)
    bool suspended = false; // Set to true if this Activity was locked out by another
    uint32_t custom_fg_color = 0; // Custom color for AT_Fingerprint2 Action
    const char* web_ping_tag = NULL;
    int web_ping_counter = 0; // Incremented on each 'ping' send to WEB, reset on each 'ping' echo from web. If this counter reached threshold limit - WEB timeout fired

    uint32_t setup_alarm_time;
    uint32_t setup_watchdog_time;

    TimerHandle_t alarm_timer = NULL;
    TimerHandle_t watchdog_timer = NULL;
    QueueHandle_t actions_queue = NULL;

    void reset_watchdog() {if (watchdog_timer) xTimerReset(watchdog_timer, portMAX_DELAY);}
    void check()
    {
        assert(status_checked);
        assert(status_ok);
    }

    void lock_out();
    void unlock_out();

    void on_alarm() {push_action_local({.type=AT_Alarm});}
    void on_watchdog() {push_action_local({.type=AT_WatchDog});}

    static void alarm_proxy(TimerHandle_t t) {((Activity*)pvTimerGetTimerID(t))->on_alarm();}
    static void watchdog_proxy(TimerHandle_t t) {((Activity*)pvTimerGetTimerID(t))->on_watchdog();}

    void push_action_local(const Action& a) 
    {
        xQueueSend(actions_queue, &a, portMAX_DELAY);
    }

    uint32_t active_actions() {return suspended ? 0 : actions | (can_be_borrowed_actions & ~borrowed);}

    void push_spc_code(int code, int arg=0) { push_action_local({.type = AT_Internal, .internal = {.code = code, .p1 = arg}});}

public:
    // Create Activity and lock all required Actions
    // If lock was unsuccessfull and AF_CanFail not specified - abort execution
    Activity(uint32_t /* bitset of ActionType and ActionFlags*/ actions, uint32_t /* bitset of ActionType*/ can_be_borrowed_actions=0);
    ~Activity();

    // Required Activity setup. Appropriate members must be called BEFORE call to get_action()
    // Multiple calls to these methods possible - later call will override setup from former.
    Activity& setup_alarm_action(uint32_t time_to_hit);
    Activity& setup_watchdog(uint32_t timeout);
    Activity& setup_web_ping_type(const char* tag) {web_ping_tag = tag; return *this;}
    Activity& set_special_color_feedback_code(uint32_t color) {custom_fg_color = color; return *this;}

    // You SHOULD call this method if AF_CanFail was specified in Activity::Activity(). Otherwise get_action will fail with abort()
    // Method returns true if this Activity successfully lock all required Actions.
    bool is_ok() {status_checked = true; return status_ok;}

    Action get_action(); //Return input action. Blocks until action will be available.

    // These callbacks will be called on Action borrow/return
    virtual void on_action_borrow(ActionType) {}
    virtual void on_action_return(ActionType) {}

    // These callbacks will be called on Override (by other Action)
    virtual void on_suspend() {}
    virtual void on_resume(LCD&); // You should restore LCD screen in this callback

    // Initialize all Activity system, starts background tasks which handles Touch/LCD and FG
    // After this call all access to LCD/Touch/FG only through LCDAccess/FPAccess
    static void start();

////////////
// Method(s) for Activity sources
    static void push_action(const Action&); // Global entry - dispatched for particular Activity inside
    static void on_web_ping_echo(const char* tag); // Global entry - dispatched to all active WEB ping sources
    static void send_web_ping(); // Internal function - called by WEB ping thread in this module.
};


class LCDAccess {
public:
    LCDAccess(Activity*);
    ~LCDAccess();

    LCD& access();
};

class FPAccess {
public:
    FPAccess(Activity*);
    ~FPAccess();

    R503& access();
};
