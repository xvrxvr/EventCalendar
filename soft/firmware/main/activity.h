#pragma once

enum ActionType {
    AT_Fingerprint     = 0x0001,   // fingerprint with normal color feedback
    AT_Fingerprint0    = 0x0003,   // fingerprint without color feedback
    AT_Fingerprint1    = 0x0005,   // fingerprint with OOB color feedback
    AT_Fingerprint2    = 0x0007,   // fingerprint with special color feedback
    AT_Alarm           = 0x0008,
    AT_TouchDown       = 0x0010,
    AT_TouchUp         = 0x0020,
    AT_TouchTrack      = 0x0040,
    AT_Touch           = TouchDown | TouchUp | TouchTrack,
    AT_Timeout         = 0x0080,
    AT_WEBEvent        = 0x0100,

    ATLast
};

enum ActionFlags {
    AF_CanFail          = 0x80000000,   // Constructor can fail - not all required ActionTypes available
    AF_Override         = 0x40000000    // Suspend current Action for duration of new one

    AFLast
};

static_assert(AFLast > ATLast, "ActionFlags and ActionType overlaped!");


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
    };
};

class Activity {
public:
    // Create Activity anl lock all required Actions
    // If lock was unsuccessfull and AF_CanFail not specified - abort execution
    Activity(int /* bitset of ActionType and ActionFlags*/ actions, int /* bitset of ActionType*/ can_be_borrowed_actions=0);
    ~Activity();

    // Required Activity setup. Appropriate members must be called BEFORE call to get_action()
    // Multiple calls to these methods possible - later call will override setup from former.
    bool setup_alarm_action(uint32_t time_to_hit);
    bool setup_timeout(uint32_t timeout);
    bool setup_web_ping_type(const char* tag);
    void set_special_color_feedback_code(int);

    // You SHOULD call this method if AF_CanFail was specified in Activity::Activity(). Otherwise get_action will fail with abort()
    // Method returns true if this Activity successfully lock all required Actions.
    bool is_ok() const;

    Action get_action();

    // These callbacks will be called on Action borrow/return
    virtual void on_action_borrow(ActionType) {}
    virtual void on_action_return(ActionType) {}

    // These callbacks will be called on Override (by other Action)
    virtual void on_suspend() {}
    virtual void on_resume(LCD&); // You should restore LCD screen in this callback
};


class LCDAccess {
public:
    LCDAccess(Activity*);
    ~LCDAccess();

    LCD& access();
};