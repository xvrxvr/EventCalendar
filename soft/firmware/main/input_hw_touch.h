#pragma once
#include "input_hw.h"
#include "pins.h"
#include "setup_data.h"
#include "activity.h"

class TouchInput : public PinAttachedInputProxy {
    int last_x=0, last_y=0;
    uint32_t touch_cmd = 0;

    bool touch_read(); // Read Touch to last_x & last_y. Returns false if Touch was lost during read (Debouncer and other will be updated)
    void push_action(ActionType); // Send action to Activity manager
    void new_touch_cmd(int cmd) {if (touch_cmd != cmd) {touch_cmd = cmd; pin_state_process(true);}}

    // Callbacks to parent
    virtual bool ll_is_enabled() override {return touch_cmd != 0;}
    virtual void ll_enable(bool enable) override {TouchConfig::raw_touch_config(enable ? TT_Sence : TT_LCD);}
    virtual int ll_tracking_time() override {return (touch_cmd & AT_TouchTrack) ? SC_TouchTrackInterval : 0;}

    // Actions from parent
    virtual void event_press() override {if (touch_read()) push_action(AT_TouchDown);}
    virtual void event_track() override;
    virtual void event_release() override {push_action(AT_TouchUp);}

public:
    TouchInput(size_t stack_size=hw_input_default_stack_size) : PinAttachedInputProxy(PIN_NUM_LCD_RS, "Touch", stack_size) {}

    // InputProxy outgoing API
    virtual void process_cmd(uint32_t cmd) override {new_touch_cmd(cmd & AT_TouchPure);}
    virtual void passivate() override {new_touch_cmd(0);}
};
