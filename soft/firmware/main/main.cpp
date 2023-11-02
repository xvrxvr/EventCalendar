#include "common.h"

#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "activity.h"

void start_http_servers();

void cross(uint16_t x, uint16_t y, uint16_t color);
    
static constexpr int RED   = 0xF800;  // BLUE?
static constexpr int GREEN = 0x07E0;  // RED?
static constexpr int BLUE  = 0x001F;  // GREEN?

static constexpr int D = 50;

static char buf[64];

#define P(line, ...) do {sprintf(buf, __VA_ARGS__); lcd.text2(buf, 0, line*32);} while(0)

bool t(int& x, int& y)
{
    TouchConfig tc;
    bool result = tc.raw_touch_read();
    x=touch_setup.x(tc.x, tc.y);
    y=touch_setup.y(tc.x, tc.y);
    return result;
}

void simple_test()
{
    for(;;)
    {
        {TouchConfig().wait_press();}
        lcd.DRect(0, 0, 10000, 10000, 0);

        int min_x = 100000, max_x = -1, min_y = 1000000, max_y = -1;
        int prev_x=-1, prev_y=-1;

        int x, y;
        while(t(x,y))
        {
//            printf("P: x=%d, y=%d\n", x, y);
            if (prev_x != -1) cross(prev_x+D, prev_y+D, 0);
            prev_x = x; prev_y = y;
            cross(x+D, y+D, GREEN);
            cross(x, y, -1);
            min_x = std::min(min_x, x); max_x = std::max(max_x, x);
            min_y = std::min(min_y, y); max_y = std::max(max_y, y);

            vTaskDelay(20);
        }
        int mx = (max_x+min_x)/2;
        int my = (max_y+min_y)/2;
        printf("Released!\nX=%d-%d (D=%d, A=%d)\nY=%d-%d (D=%d, A=%d)\n", min_x, max_x, max_x-min_x, mx, min_y, max_y, max_y-min_y, my);
        P(0, "X=%d-%d (D=%d, A=%d)", min_x, max_x, max_x-min_x, mx);
        P(1, "Y=%d-%d (D=%d, A=%d)", min_y, max_y, max_y-min_y, my);
        cross(mx, my, RED);
        TouchConfig().wait_release();
    }
}

class MyActivity : public Activity {
public:
    using Activity::Activity;

    virtual void update_scene(LCD&) {}
};

void test1()
{
    MyActivity act(AT_Fingerprint|AT_TouchDown|AT_TouchTrack|AT_TouchUp|AT_WatchDog);
    act.setup_watchdog(10); // 10 seconds
    for(;;)
    {
        auto action = act.get_action();
        printf("Action: %x\n",action.type);
        if (action.type & AT_Fingerprint) printf(" FP: %d\n", action.fp_index);
        if (action.type & AT_TouchDown)
        {
            printf(" Touch: %d,%d\n", action.touch.x, action.touch.y);
            Activity::LCDAccess access(&act);
            P(0, "%d, %d", action.touch.x, action.touch.y);
            cross(action.touch.x, action.touch.y, -1);
        }
        if (action.type & AT_WatchDog) printf("  WatchDog\n");
    }
}

void send_web_ping_to_ws(const char*) {}

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();
    start_http_servers();
    Activity::start();

//    touch_setup.calibrate();
//    printf("Setup=%ld,%ld,%ld,%ld,%ld,%ld\n", touch_setup.A, touch_setup.B, touch_setup.C, touch_setup.D, touch_setup.E, touch_setup.F);

    lcd.text("Hello!", 100, 100);

 //   simple_test();
    test1();

    for(;;);
}