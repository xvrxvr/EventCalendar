#pragma once

#define LCD_AP_ROW 0
#define LCD_STA_ROW 3
#define LCD_MSG_ROW 5
#define LCD_MSG_PI 6

inline constexpr uint16_t rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    red>>=3;
    green>>=2;
    blue>>=3;
    return (uint16_t(red) << 11) | (uint16_t(green)<<5) | blue;
}

class LCD {
    int _FlipMode;
    int _res_x;
    int	_res_y;

    uint8_t fc_high=0xFF, fc_low=0xFF, bc_high=0, bc_low=0;

    void LCD_CtrlWrite_ILI9327(uint8_t command, uint8_t data);

    void _WRITE_DATA(uint8_t d);
    void _WriteCommand(uint8_t command);

    void _SetWriteArea(int16_t& X1, int16_t& Y1, int16_t& X2, int16_t& Y2);
    void _SetWriteAreaDelta(int16_t& X1, int16_t& Y1, int16_t dx, int16_t dy);

    void WImgPalleteClip(int16_t x1, int16_t y1, int16_t w, int16_t h, const uint8_t* image, const uint16_t* pallete);

    int msg_x=0, msg_y=0;
    int loaded=0;

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
    void WImgPallete(int16_t x1, int16_t y1, int16_t w, int16_t h, const uint8_t* image, const uint16_t* pallete, bool with_clip=false);

    // Draw buffer in format RGB888
    void draw_rgb888(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t* data);

    void set_fg(uint16_t color) {fc_high = color >> 8; fc_low = color & 0xFF;}
    void set_bg(uint16_t color) {bc_high = color >> 8; bc_low = color & 0xFF;}

    // Emit text. Use -1 for x coordinate to emit in center of screen
    void text(const char* text, int16_t x, int16_t y, int length=-1);
    void text2(const char* text, int16_t x, int16_t y, int length=-1);

    // Print error message in red
    void error(const char* text)
    {
        set_fg(rgb(0xFF, 0, 0));
        status(text);
        set_fg(rgb(0xFF, 0xFF, 0xFF));
    }

    // Print status message
    void status(const char* text)
    {
        gotoxy(LCD_MSG_ROW);
        print(text);
    }

    // Terminal mode prints
    void gotoxy(int row, int col=0) {msg_x=col; msg_y=row;}
    void print(const char*); // \n also handled

    // Progress indicator
    void pi_inc_loaded(size_t);
};

/*  50x15  or 25x7.5       V
 0: SSID: EventCalendar < White
 1: PSWD: CalendarEvent < White
 2: IP  : 192.168.4.1   < White
 3: IP : ....           < Green
 4: AKA: ....           < Green
 5: Waiting for update  < White/Red
 6: .. Progress indicator .. (80x12)

*/

extern LCD lcd;

void hw_init();

struct EEPROM {
    static constexpr int page_size = 32;

    // Page writes performed on page boundaries - page write will wrap around if address+size will cross 32 byte block
    static void read(uint16_t address, void* data, uint8_t size);
    
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
    static void read(int page_address, Data& data) {read_pg(page_address, &data, sizeof(data));}
    
private:
    static void poll4ready();
};
