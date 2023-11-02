#pragma once

#include "R503.h"

class LCD {
    int _FlipMode;
    int _res_x;
    int	_res_y;

    uint8_t fc1=0xFF, fc2=0xFF, bc1=0, bc2=0;

    void LCD_CtrlWrite_ILI9327(uint8_t command, uint8_t data);

    void _WRITE_DATA(uint8_t d);
    void _WriteCommand(uint8_t command);

    void _SetWriteArea(int16_t& X1, int16_t& Y1, int16_t& X2, int16_t& Y2);
    void _SetWriteAreaDelta(int16_t& X1, int16_t& Y1, int16_t dx, int16_t dy);

public:
    static const int16_t MAX = 0x7FFF; // Maximum coordinate

    void write_lcd(uint8_t d);

    enum {
        SCREEN_NORMAL,
        SCREEN_R90,
        SCREEN_R180,
        SCREEN_R270,

        SCREEN_MY = SCREEN_R270
    };

    void init_screen(void);

    void DReset(void);

    /* Places the screen in to or out of sleep mode where:
        mode is the required state. Valid values area
            true (screen will enter sleep state)
            false (screen will wake from sleep state)
    */
    void DSleep(bool mode);

    /* Turn the screen on or off where:
        mode is the required state. Valid values are
            ON (screen on)
            OFF (screen off)
    */
    void DScreen(bool mode);

    /* Sets the screen orientation and write direction where:
        mode is the direction, orientation to set the screen to. Valid vales are:
            SCREEN_NORMAL 		(Default)
            SCREEN_R90 			Screen is rotated 90o		
            SCREEN_R180 		Screen is rotated 180o
            SCREEN_R270 		Screen is rotated 270o		
    */
    void DFlip(uint8_t mode);

    /* Draws a solid rectangle to the using the current foreground colour where:
        x1 is the x coordinate of the top left corner of the rectangle.
        y1 is the y coordinate of the top left corner of the rectangle.
        x2 is the x coordinate of the bottom right corner of the rectangle.
        y2 is the y coordinate of the bottom right corner of the rectangle.
    */
    void DRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void WRect(int16_t x1, int16_t y1, int16_t w, int16_t h, uint16_t color) {DRect(x1, y1, x1+w-1, y1+h-1, color);}

    void set_fg(uint16_t color) {fc1 = color >> 8; fc2 = color & 0xFF;}
    void set_bg(uint16_t color) {bc1 = color >> 8; bc2 = color & 0xFF;}

    // Emit text. Use -1 for x coordinate to emit in center of screen
    void text(const char* text, int16_t x, int16_t y);
    void text2(const char* text, int16_t x, int16_t y);
};

extern R503 fp_sensor;
extern LCD lcd;
extern volatile uint8_t time_to_reengage_sol;

void hw_init();

void sol_hit(int index);

//    _BGColour = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
//       <rrrrr> <ggg> | <ggg> <bbbbb>

// RAW touch
enum TouchType {
    TT_LCD,
    TT_X,
    TT_XInv,
    TT_Y,
    TT_YInv,
    TT_Sence
};

struct TouchConfig {
    int x, y; // Raw X & Y of tounch (if any) or -1, -1 if no touch

    TouchConfig();
    ~TouchConfig();
    
    // Update current state and returns true if touched (x & y filled with data)
    bool touched();
    
    void wait_press();
    void wait_release();

    static int raw_touch_config(TouchType tt);

    // Read Touch. Return true if Touch still pressed at end
    static bool raw_touch_read(int& x, int& y);
    bool raw_touch_read() {return raw_touch_read(x, y);}
};


int get_temperature();

struct RTC {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;

    bool read(); // return true if RTC is started
    void write();
    void start();
    void read_ram(void* dst, uint8_t shift, uint8_t size);
    void write_ram(const void* src, uint8_t shift, uint8_t size);
};


struct EEPROM {
    static constexpr int page_size = 32;

    // Page writes performed on page boundaries - page write will wrap around if address+size will cross 32 byte block
    static void write(uint16_t address, const void* data, uint8_t size);
    static void read(uint16_t address, void* data, uint8_t size);
    
    static void write_pg(int page_addr, const void* data, uint16_t size)
    {
        uint16_t a = page_addr * page_size;
        const uint8_t* p = (const uint8_t*)data;
        while(size)
        {
            uint8_t len = std::min<uint16_t>(page_size, size);
            write(a, p, len);
            a+=page_size;
            p += len;
            size -= len;
        }
    }
    
    static void read_pg(int page_addr, void* data, uint16_t size)
    {
        uint16_t a = page_addr * page_size;
        uint8_t* p = (uint8_t*)data;
        while(size)
        {
            uint8_t len = std::min<uint16_t>(page_size, size);
            read(a, p, len);
            a+=page_size;
            p += len;
            size -= len;
        }
    }
    
    template<class Data>
    static void write(int page_address, const Data& data) {write_pg(page_address, &data, sizeof(data));}
    template<class Data>
    static void read(int page_address, Data& data) {read_pg(page_address, &data, sizeof(data));}
    
    
private:
    static void poll4ready();
};

void fade_out();

void fade_in();

bool test_wakeup();

//esp_lcd_panel_handle_t esp_lcd_new_panel_my();
