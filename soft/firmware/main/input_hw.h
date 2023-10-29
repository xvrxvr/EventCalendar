#pragma once

#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "common.h"

class InputProxy {
    const char* tag;
    size_t stack_size;

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
        while(xQueueReceive(data_queue, &cmd, 0)) process_cmd(cmd);
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
            xTaskNotifyWait(0, -1, &notify_val, portMAX_DELAY);
            pass_cmds();
            if (notify_val & B_WakeUp) process_input();
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
    InputProxy(const char* tag, size_t stack_size=1024) :tag(tag), stack_size(stack_size) {}

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

    [[gnu::always_inline]] void wakeup() 
    {
        BaseType_t prio = 0;
        xTaskNotifyFromISR(worker, B_WakeUp, eSetBits, &prio);
    }

    virtual void init() = 0; // Initialize all hardware
    virtual void enable() = 0; // Enable interrupts from Input HW
    virtual void disable() = 0; // Disable interrupts from Input HW
    virtual void process_input() = 0; // Called on interrupt from Input HW
    virtual void process_cmd(uint32_t) = 0; // Called to process external command
};

class TouchInput : public InputProxy {
public:
    using InputProxy::InputProxy;

    virtual void init() override; // Initialize all hardware
    virtual void enable() override; // Enable interrupts from Input HW
    virtual void disable() override; // Disable interrupts from Input HW
    virtual void process_input() override; // Called on interrupt from Input HW
    virtual void process_cmd(uint32_t) override; // Called to process external command
};

class FGInput : public InputProxy {
public:
    using InputProxy::InputProxy;

    virtual void init() override; // Initialize all hardware
    virtual void enable() override; // Enable interrupts from Input HW
    virtual void disable() override; // Disable interrupts from Input HW
    virtual void process_input() override; // Called on interrupt from Input HW
    virtual void process_cmd(uint32_t) override; // Called to process external command
};
