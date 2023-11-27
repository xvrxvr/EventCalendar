#pragma once

#include "hadrware.h"

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
    AT_TouchPure       = (AT_TouchDown|AT_TouchUp|AT_TouchTrack) & ~AT_TouchBit,
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

inline int operator|(ActionType at, ActionFlags af) {return int(at)|int(af);}

enum WebEvents {
    WE_Login,     // No params
    WE_Logout,    // logout_tag
    WE_GameStart, // No params
    WE_GameEnd,   // No params
    WE_FGDel,     // p1 - <User-index>*4 + <FG-index-in-lib> or p1 - <FG-index-in-lib> | 0x1000
    WE_FGEdit,    // p1 - User index (-1 - new user)
    WE_FGView,

    // FG Editor only events
    WE_FGE_Done,  // Done editor. p1 - new user age (or -1), p2 - new user name (DOS) or NULL
};

struct Action {
    ActionType type;
    union {
        struct {
            int x, y;
        } touch;
        struct {
            int16_t fp_index;
            uint16_t fp_score;
            const char* fp_error;
        };
        struct {
            WebEvents event;
            union {
                int p1;
                const char* logout_tag;
            };
            const char* p2;
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
    std::atomic<uint32_t> borrowed = 0; // Set of Actions that was really borrowed
    uint32_t locked_out = 0; // Set of indexes of other Activities that was locked out by this one (by AF_Override option)
    std::atomic<bool> suspended = false; // Set to true if this Activity was locked out by another
    uint32_t custom_fg_color = auraLEDCode(ALC_Breathing, ALC_Purpule); // Custom color for AT_Fingerprint2 Action
    const char* web_ping_tag = NULL;
    int web_ping_counter = 0; // Incremented on each 'ping' send to WEB, reset on each 'ping' echo from web. If this counter reached threshold limit - WEB timeout fired
    bool update_scene_req = false; // Set to true to update Scene before waiting for Action

    uint32_t setup_alarm_time;
    uint32_t setup_watchdog_time = 0;

    TimerHandle_t alarm_timer = NULL;
    QueueHandle_t actions_queue = NULL;

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
    Activity& setup_alarm_action(time_t time_to_hit); // time_to_hit is UTC timestamp
    Activity& setup_watchdog(uint32_t timeout) {setup_watchdog_time = timeout; return *this;} // time in seconds
    Activity& setup_web_ping_type(const char* tag) {web_ping_tag = tag; return *this;}
    Activity& set_special_color_feedback_code(uint32_t color) {custom_fg_color = color; return *this;}

    // You SHOULD call this method if AF_CanFail was specified in Activity::Activity(). Otherwise get_action will fail with abort()
    // Method returns true if this Activity successfully lock all required Actions.
    bool is_ok() {status_checked = true; return status_ok;}

    // Request to update current Scene
    void update() {update_scene_req = true;}

    Action get_action(); //Return input action. Blocks until action will be available.

    // These callbacks will be called on Action borrow/return
    virtual void on_action_borrow(ActionType) {}
    virtual void on_action_return(ActionType) {}

    // These callbacks will be called on Override (by other Action)
    virtual void on_suspend() {}
    virtual void on_resume() {} // You should restore LCD screen in this callback

    // Scene updater
    virtual void update_scene(LCD&) {}

    // Initialize all Activity system, starts background tasks which handles Touch/LCD and FG
    // After this call all access to LCD/Touch/FG only through LCDAccess/FPAccess
    static void start();

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

////////////
// Method(s) for Activity sources
    static void push_action(const Action&); // Global entry - dispatched for particular Activity inside
    static void on_web_ping_echo(const char* tag); // Global entry - dispatched to all active WEB ping sources
    static void send_web_ping(); // Internal function - called by WEB ping thread in this module.
};
