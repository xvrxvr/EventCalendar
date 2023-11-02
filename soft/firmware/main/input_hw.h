#pragma once

static constexpr int hw_input_default_stack_size = 8*1024;

struct Action;

class InputProxy {
    const char* tag;
    size_t stack_size;
    bool passivate_pending = false;
    uint32_t autorepeat = 0;

    TaskHandle_t worker = NULL;
    SemaphoreHandle_t outer_lock = NULL;
    SemaphoreHandle_t inner_lock = NULL;
    QueueHandle_t data_queue = NULL;

    enum {
        B_AqReq = 1,
        B_AqAns = 2,
        B_DataAvail  = 4,
        B_WakeUp     = 8
    };

    void pass_cmds()
    {
        uint32_t cmd;
        while(xQueueReceive(data_queue, &cmd, 0))
        {
            passivate_pending = false;
            process_cmd(cmd);
        }
    }

    void run()
    {
        uint32_t notify_val;
        xSemaphoreTake(inner_lock, portMAX_DELAY);
        init();
        enable();
        for(;;)
        {
            pass_cmds();
            TickType_t wait_time = autorepeat ? autorepeat : passivate_pending ? ms2ticks(SC_HW_INPUT_AUTO_OFF) : portMAX_DELAY;
            xTaskNotifyWait(0, -1, &notify_val, wait_time);
            pass_cmds();
            if (!notify_val)
            {
                if (autorepeat) process_autorepeat(); else
                if (passivate_pending) {passivate_pending = false; passivate();}
                continue;
            }
            if (notify_val & B_WakeUp) {process_input();}
            if (notify_val & B_AqReq) // Lock processing
            {
                disable();
                xSemaphoreGive(inner_lock);
                do {xTaskNotifyWait(0, -1, &notify_val, portMAX_DELAY);} while (!(notify_val & B_AqAns));
                xSemaphoreTake(inner_lock, portMAX_DELAY);
                enable();
            }
        }
    }

    static void worker_proxy(void* self)
    {
        ((InputProxy*)self)->run();
    }

public:
    InputProxy(const char* tag, size_t stack_size=hw_input_default_stack_size) :tag(tag), stack_size(stack_size) {}

    void start() // Start input task
    {
        outer_lock = xSemaphoreCreateMutex();
        inner_lock = xSemaphoreCreateMutex();
        data_queue = xQueueCreate(5, sizeof(uint32_t));
        xTaskCreate( &worker_proxy, tag, stack_size, this, TP_Hardware, &worker);
    }

    bool is_started() const {return worker != NULL;}
    void suspend() // Suspend input task, release hardware to external control
    {
        xSemaphoreTake(outer_lock, portMAX_DELAY);
        xTaskNotify(worker, B_AqReq, eSetBits);
        xSemaphoreTake(inner_lock, portMAX_DELAY);
        xTaskNotify(worker, B_AqAns, eSetBits);    
    }

    void resume() // Resume input task, re-aquire hardware
    {
        xSemaphoreGive(inner_lock);
        xSemaphoreGive(outer_lock);
    }

    void cmd(uint32_t c) // Pass external command to Input task
    {
        xQueueSendToBack(data_queue, &c, portMAX_DELAY);
        xTaskNotify(worker, B_DataAvail, eSetBits);
    }

    void set_autorepeat(uint32_t rep_interval_in_ticks=0) {autorepeat = rep_interval_in_ticks;}

    // Wakeup (and call process_input() ) from ISR
    [[gnu::always_inline]] void wakeup_from_isr() 
    {
        BaseType_t prio = 0;
        xTaskNotifyFromISR(worker, B_WakeUp, eSetBits, &prio);
    }

protected:
    // Callbacks. Intended to call from child only
    virtual void init() = 0; // Initialize all hardware
    virtual void enable() = 0; // Enable interrupts from Input HW
    virtual void disable() = 0; // Disable interrupts from Input HW
    virtual void process_input() = 0; // Called on interrupt from Input HW
    virtual void process_cmd(uint32_t) = 0; // Called to process external command
    virtual void process_autorepeat() = 0; // Called at autorepeat intervals
    virtual void passivate() = 0; // Called when no new commands arrived in SC_HW_INPUT_AUTO_OFF interval

    void push_action(const Action&); // Implementation in activity.cpp
};

// HW input proxy for HW attached to GPIO pin
// Interrupt handler installed automatically and call wakeup_from_isr() and disable interrupt (it should be reenabled in process_input() or some other callback)
class SimplePinAttachedInputProxy : public InputProxy {
    static void IRAM_ATTR isr_handler(void* arg)
    {
        SimplePinAttachedInputProxy* self = (SimplePinAttachedInputProxy*)arg;
        self->wakeup_from_isr();
        gpio_intr_disable(self->gpio_pin_num);
    }
protected:
    gpio_num_t gpio_pin_num;

    // Check pin state, returns logical state. Default implementation return pin state directly (pin logic assumed to be inverted)
    bool ll_pin_state() {return !gpio_get_level(gpio_pin_num);}

    void ei() {gpio_intr_enable(gpio_pin_num);}
    void di() {gpio_intr_disable(gpio_pin_num);}

public:
    SimplePinAttachedInputProxy(gpio_num_t gpio_pin_num, const char* tag, size_t stack_size=hw_input_default_stack_size) : InputProxy(tag, stack_size), gpio_pin_num(gpio_pin_num) {}

protected:
    virtual void init() override // Initialize all hardware
    {
        gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        gpio_isr_handler_add(gpio_pin_num, &isr_handler, this);
    }
};

// HW input proxy for HW attached to GPIO pin
// Interrupt handler installed automatically and call wakeup_from_isr() and disable interrupt (it should be reenabled in process_input() or some other callback)
// Also class implements debounce and convert pin state to events
class PinAttachedInputProxy : public SimplePinAttachedInputProxy {
    Debouncer deb;

protected:

    // Main member to process all changes in pin state
    void pin_state_process(bool clear_debouncer = false)
    {
        if (clear_debouncer) deb.clear();

        if (!ll_is_enabled())
        {
            set_autorepeat(0);
            di();
            return;
        }

        const int track_time = ll_tracking_time();
        const bool was_stable = deb.stable();

        deb << ll_pin_state();
        if (!deb.stable()) // Not stable - activate autorepeat for debounce purpose
        {
            set_autorepeat(ll_debounce_time());
            di();
        }
        else if (was_stable) // Stable state (no change) - on or off
        {
            if (!deb.value() || !track_time) // Off or no tracking
            {
                set_autorepeat();
                ei();
            }
            else // Tracking
            {
                event_track();
                set_autorepeat(track_time);
                di();
            }
        }
        else if (deb.value()) // turn on
        {
            event_press();
            set_autorepeat(track_time); // Turn on/off repeat for Tracking
            if (track_time) di(); else ei();
        }
        else // turn off
        {
            event_release();
            set_autorepeat();
            ei();
        }
    }

    /////////// Low level interface (to be implemented in derived class) ////////////
    
    // Callabacks

    // Is HW enabled?
    virtual bool ll_is_enabled() = 0;

    // Enable/disable hardware. Default implementation assumed that hardware always enabled
    virtual void ll_enable(bool enable) {}

    // Get time interval between track events. Default implementation assume tracking is turned off
    virtual int ll_tracking_time() {return 0;}

    // Get debounce time
    /*virtual*/ int ll_debounce_time() {return SC_DebounceTime;}

    // Outgoing API
    virtual void event_press() = 0; // Pin was activated
    virtual void event_track() {}   // Tracking event
    virtual void event_release() {} // Pin was desactivated

public:
    PinAttachedInputProxy(gpio_num_t gpio_pin_num, const char* tag, size_t stack_size=hw_input_default_stack_size) : SimplePinAttachedInputProxy(gpio_pin_num, tag, stack_size) {}

protected:
    virtual void init() override // Initialize all hardware
    {
        SimplePinAttachedInputProxy::init();
        pin_state_process();
    }

    virtual void enable() override {ll_enable(true); pin_state_process();} // Enable interrupts from Input HW
    virtual void disable() override {ll_enable(false); di();} // Disable interrupts from Input HW
    virtual void process_input() override {pin_state_process();}
    virtual void process_autorepeat() override {pin_state_process();}
};
