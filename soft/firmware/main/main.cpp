#include "common.h"

#include "hadrware.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "activity.h"
#include "box_draw.h"
#include "text_draw.h"
#include "keyboard.h"
#include "interactive.h"
#include "challenge_list.h"

void start_http_servers();

#if 0
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

    virtual void update_scene(LCD&) {printf("MyActivity(%p):UpdateScene()\n", this);}
    virtual void on_action_borrow(ActionType at) {printf("MyActivity(%p):borrow(%x)\n", this, at);}
    virtual void on_action_return(ActionType at) {printf("MyActivity(%p):return(%x)\n", this, at);}

    // These callbacks will be called on Override (by other Action)
    virtual void on_suspend() {printf("MyActivity(%p):suspend()\n", this);}
    virtual void on_resume() {printf("MyActivity(%p):resume()\n", this);}
};


void test3(void*)
{
    {
        MyActivity act(AT_TouchDown|AF_Override);
        printf("Test3 Activity %p\n", &act);
        auto action = act.get_action();
        printf("Action3: %x -> exit\n",action.type);
    }
    vTaskDelete(NULL);
}

void test2(void*)
{
    {
        MyActivity act(AT_Fingerprint1);
        printf("Test2 Activity %p\n", &act);
        auto action = act.get_action();
        printf("Action2/1: %x\n",action.type);
        xTaskCreate(test3, "Test2", 8096, NULL, 5, NULL);
        action = act.get_action();
        printf("Action2/2: %x -> exit\n",action.type);
    }
    vTaskDelete(NULL);
}

void test1()
{
    MyActivity act(AT_TouchDown|AT_TouchTrack|AT_TouchUp|AT_WatchDog, AT_Fingerprint);
    printf("Test1 Activity %p\n", &act);
    act.setup_watchdog(10); // 10 seconds
    for(;;)
    {
        auto action = act.get_action();
        printf("Action: %x\n",action.type);
        if (action.type & AT_Fingerprint) 
        {
            printf(" FP: %d\n", action.fp_index);
            static auto unused = xTaskCreate(test2, "Test2", 8096, NULL, 5, NULL);
        }
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

static constexpr int R1   = 0x0800;  // BLUE?
static constexpr int G1   = 0x0020;  // RED?
static constexpr int B1   = 0x0001;  // GREEN?

static void test_color1(int y, const char* title, uint16_t color, int bits)
{
    lcd.text2(title, 0, y);
    int start = 2*16+8;
    int w = 10;
    if (bits == 6) w >>= 1;
    for(int c=0; c<(1<<bits); ++c)
    {
        lcd.WRect(start, y, w, 32, c*color);
        start+=w;
    }
}

static void test_colors()
{
    int y = 60;
    test_color1(y, "R:", R1, 5);
    test_color1(y+35, "G:", G1, 6);
    test_color1(y+70, "B:", B1, 5);
}
#endif

#if 0
#include "rom/tjpgd.h"

#define JPEG_WORK_BUF_SIZE  3100    /* Recommended buffer size; Independent on the size of the image */
//#define JD_FORMAT 0
//#define ESP_JPEG_COLOR_BYTES    3

struct MyConfig {
    const uint8_t* buffer;
    size_t rest_of_buffer;
};

static unsigned int jpeg_decode_in_cb(JDEC *dec, uint8_t *buff, unsigned int nbyte)
{
    assert(dec != NULL);

    MyConfig *cfg = (MyConfig *)dec->device;
    assert(cfg != NULL);

    if (nbyte > cfg->rest_of_buffer) nbyte = cfg->rest_of_buffer;
    if (buff) 
    {
        memcpy(buff, cfg->buffer, nbyte);
    }
    cfg->rest_of_buffer -= nbyte;
    cfg->buffer += nbyte;

    return nbyte;
}

static unsigned int jpeg_decode_out_cb(JDEC *dec, void *bitmap, JRECT *rect)
{
    lcd.draw_rgb888(rect->left, rect->top, rect->right, rect->bottom, (uint8_t*)bitmap);
    return 1;
}


void test_jpeg(const void* img, size_t size)
{
    JDEC JDEC;

    std::unique_ptr<uint8_t[]> workbuf(new uint8_t[JPEG_WORK_BUF_SIZE]);

    MyConfig cfg {
        .buffer = (const uint8_t *)img,
        .rest_of_buffer = size
    };

    /* Prepare image */
    auto err = jd_prepare(&JDEC, jpeg_decode_in_cb, workbuf.get(), JPEG_WORK_BUF_SIZE, &cfg);
    printf("jd_prepare -> %d\n", err);
    err = jd_decomp(&JDEC, jpeg_decode_out_cb, 0);
    printf("jd_decomp -> %d\n", err);
}
#endif

extern "C" void app_main(void)
{
    hw_init();
    init_or_load_setup();
    wifi_init();
    start_http_servers();
    Activity::start();

//    touch_setup.calibrate();
//    printf("Setup=%ld,%ld,%ld,%ld,%ld,%ld\n", touch_setup.A, touch_setup.B, touch_setup.C, touch_setup.D, touch_setup.E, touch_setup.F);

//    lcd.text("Hello!", 100, 100);

 //   simple_test();
//    test1();

 //   test_colors();

/*
    extern const char test_bg_start[] asm("_binary_test_bg_jpg_start");
    extern const char test_bg_end[] asm("_binary_test_bg_jpg_end");
    test_jpeg(test_bg_start, test_bg_end-test_bg_start);
*/

//     BoxCreator(int width, int height, int r, int border_width, int shadow_width);
 //   BoxCreator box(200, 100, 15, 2, 5);

//    static const uint16_t pallete[] = {0, rgb(0x04, 0xAA, 0x6D), 0, rgb(0x66,0x66,0x66)};
//    box.draw(lcd, 50, 50, pallete, true);
/*
    TextBoxDraw::TextGlobalDefinition gd;
    TextBoxDraw::TextsParser pars(gd);
    pars.parse_text("Hello world!\n\\2\\#Hi!");
    pars.draw_one_box_centered(lcd);
*/

//    uint32_t door = open_door(0);
//    printf("OpenDoor returns %lX\n", door);

    printf("EQuest = %d\n", EQuest::run_challenge());

    for(;;) {Interactive::entry(); vTaskDelay(100);}

    GridManager::KeybBoxDef bdef{
        .box_def{
            .box_defs = {
                "5555155U0A630E03 D6D9,0000,9CD3,FFFF", // Box
                "2211003U0A630E03 1BE3,0000,9CD3,FFFF" // Keyboard
            },
            .reserve_top = 20, // -1 for reserve all availabe space
            .reserve_left = 0, // -1 for reserve all availabe space
            .cell_width = 0, // Set to >0 value to define internal cell width. By default defined by cell contents
            .cell_height = 16 // Define internal cell height.
        }
    };
    Activity act(AT_TouchDown|AT_WatchDog);
    act.setup_watchdog_ticks(50);

#define lcd Activity::LCDAccess(&act).access()

    GridManager::Keyboard kb(bdef, GridManager::keyb_combo_eng, GridManager::keyb_combo_rus, 1);
    kb.set_coord(lcd, GridManager::Rect{0, 0, 400, 240});
    kb.kb_activate(lcd);
    for(;;)
    {
        auto a = act.get_action();
        if (a.type == AT_TouchDown)
        {
            int id = kb.get_touch(a.touch.x, a.touch.y).id;
            if (id == -1) continue;
            if (kb.kb_process(lcd, id))
            {
                printf("Collected '%s'\n", kb.kb_get_string());
            }
        } else
        if (a.type == AT_WatchDog) kb.kb_animate(lcd);
    }

}