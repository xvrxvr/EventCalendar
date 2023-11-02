#pragma once
#include "input_hw.h"
#include "pins.h"


class FGInput : public PinAttachedInputProxy {
    int prev_color = -1;

    void restart() {pin_state_process(true);}

    virtual void event_press() override;
    virtual bool ll_is_enabled() override {return prev_color != -1;}
public:
    FGInput(size_t stack_size=hw_input_default_stack_size) : PinAttachedInputProxy(PIN_NUM_FP_WAKEUP, "InputFG", stack_size) {}

    // InputProxy outgoing API
    virtual void process_cmd(uint32_t) override; // Called to process external command
    virtual void passivate() override; // Called when no new commands arrived in SC_HW_INPUT_AUTO_OFF interval
};
