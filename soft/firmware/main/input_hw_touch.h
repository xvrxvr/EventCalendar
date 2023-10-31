#pragma once
#include "input_hw.h"
#include "pins.h"

class TouchInput : public PinAttachedInputProxy {
    Debouncer db;
    bool prev_pressed = false;
    int last_x=0, last_y=0;
    uint32_t touch_cmd = 0;
    bool in_db = false; // Do we in Debouncer process (or in Move Track)

    bool touch_read(); // Read Touch to last_x & last_y. Returns false if Touch was lost during read (Debouncer and other will be updated)

    void process_press();
    void process_release();
    void process_autorep();

    void process_input_signal(bool is_autorepeat);
public:
    TouchInput(size_t stack_size=hw_input_default_stack_size) : PinAttachedInputProxy(PIN_NUM_LCD_RS, "Touch", stack_size) {}

//    virtual void init() override; // Initialize all hardware
    virtual void enable() override; // Enable interrupts from Input HW
    virtual void disable() override; // Disable interrupts from Input HW
    virtual void process_input() override {process_input_signal(false);} // Called on interrupt from Input HW
    virtual void process_cmd(uint32_t) override; // Called to process external command
    virtual void process_autorepeat() override {process_input_signal(true);}; // Called at autorepeat intervals
    virtual void passivate() override; // Called when no new commands arrived in SC_HW_INPUT_AUTO_OFF interval
};
