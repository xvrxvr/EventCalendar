#pragma once
#include "input_hw.h"
#include "pins.h"


class FGInput : public PinAttachedInputProxy {
    Debouncer db;
    bool prev_pressed = false;
    uint32_t prev_color = -1;

    bool upd_finger_detector();
    void check_press();
    void restart();

public:
    FGInput(size_t stack_size=hw_input_default_stack_size) : PinAttachedInputProxy(PIN_NUM_FP_WAKEUP, "InputFG", stack_size) {}

    virtual void enable() override; // Enable interrupts from Input HW
    virtual void process_input() override {check_press();} // Called on interrupt from Input HW
    virtual void process_cmd(uint32_t) override; // Called to process external command
    virtual void process_autorepeat() override {check_press();} // Called at autorepeat intervals
    virtual void passivate() override; // Called when no new commands arrived in SC_HW_INPUT_AUTO_OFF interval
};
