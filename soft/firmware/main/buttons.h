#pragma once

#include "text_draw.h"

struct ButtonsSetup {
    const TextBoxDraw::TextGlobalDefinition not_pressed;  // Setup for not pressed button
    uint16_t fg_color_pressed = 0;                        // FG color for pressed button (rgb565 hex string)
    uint16_t bg_color_pressed = 0xAFD5;                   // BG color for pressed button (rgb565 hex string)
    uint16_t gray_area_color = rgb(0x80, 0x80, 0x80);     // Color of circle area around button
    uint8_t gray_area_width = 2;                          // Width/Height (in pixel) or circle area around button
    uint8_t press_shift = 3;                              // Shift (x/y) of image of pressed button relative unpressed one
    uint8_t feedback_time = 3;                            // Time of momentary visual feedback of button pressed (in 0.1 sec)

    ButtonsSetup(const TextBoxDraw::TextGlobalDefinition& np) : not_pressed(np) {}
};
extern ButtonsSetup global_buttons_setup;

class Buttons {
    static constexpr int MAX_ROWS = 4;

    enum ButtonOption : uint8_t {
        BO_None,
        BO_RadioButton  = 0x01,
        BO_ShowUpArrow  = 0x02,
        BO_ShowDnArrow  = 0x04
    };

    const ButtonsSetup& bs;
    Prn buf;
    TextBoxDraw::CellDef cdefs[MAX_ROWS];
    std::vector<int> buttons_indexes;
    uint16_t box_x;
    uint16_t box_w;
    uint8_t box_y;
    uint8_t box_h;

    uint8_t cur_line = 0;
    int8_t cur_pressed = -1;
    /*ButtonOption*/ uint8_t options = BO_None;


    TextBoxDraw::TextGlobalDefinition get_pressed_setup() const
    {
        TextBoxDraw::TextGlobalDefinition result = bs.not_pressed;
        result.fg_color = bs.fg_color_pressed;
        result.bg_color = bs.bg_color_pressed;
        result.shadow_width -= bs.press_shift;
        return result;
    }

    void init(bool radiobuttons)
    {
        if (radiobuttons) options |= BO_RadioButton;
        buf.strcpy("\x1E\x1E\x1E\n\x1F\x1F\x1F\n"); // or 18/19
    }

    int total_boxes() const {return std::min<int>(MAX_ROWS, buttons_indexes.size());}

    bool is_scroll_active() const {return buttons_indexes.size() > MAX_ROWS;}

    void draw_gray_border();

    void fill_cdefs_indexes();
    void scroll_to(int start_index);
    void draw_one(int screen_index, bool pressed);
    void visual_feedback(int screen_index)
    {
        draw_one(screen_index, true);
        vTaskDelay(ms2ticks(bs.feedback_time*100));
        draw_one(screen_index, false);
    }

    int index2screen(int index)
    {
        if (index == -1 || index < cur_line) return -1;
        if (index - cur_line >= MAX_ROWS - (options & BO_ShowUpArrow ? 1 : 0) - (options & BO_ShowDnArrow ? 1 : 0)) return -1;
        index -= cur_line;
        if (options & BO_ShowUpArrow) ++index;
        return index;
    }

    int press_imp(int);

public:
    Buttons(const ButtonsSetup& bs, bool radiobuttons) : bs(bs) {init(radiobuttons);}
    Buttons(bool radiobuttons) : bs(global_buttons_setup) {init(radiobuttons);}

    Buttons& add_button(const char* text, int value, bool pressed=false)
    {
        buf.cat_printf("%s\n", text);
        if (pressed) cur_pressed = buttons_indexes.size();
        buttons_indexes.push_back(value);
        return *this;
    }

    void draw(int x, int y, int w, int h);

    enum {
        PressNo= -1,
        PressInternal = -2
    };

    int press(int x, int y);
    int get_currently_pressed() const {return cur_pressed == -1 ? -1 : buttons_indexes[cur_pressed];}
};
