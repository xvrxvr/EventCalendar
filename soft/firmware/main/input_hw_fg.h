#pragma once
#include "input_hw.h"
#include "pins.h"


class FGInput : public SimplePinAttachedInputProxy {
    int prev_color = -1;

    void press();

public:
    FGInput(size_t stack_size=hw_input_default_stack_size) : SimplePinAttachedInputProxy(PIN_NUM_FP_WAKEUP, "InputFG", stack_size) {}

    // InputProxy outgoing API
    virtual void enable() override {ei();} // Enable interrupts from Input HW
    virtual void disable() override {di();} // Disable interrupts from Input HW
    virtual void process_input() override {press(); ei();}
    virtual void process_cmd(uint32_t) override; // Called to process external command
    virtual void passivate() override; // Called when no new commands arrived in SC_HW_INPUT_AUTO_OFF interval
    virtual void process_autorepeat() override {}
};
