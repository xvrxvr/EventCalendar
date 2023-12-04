#pragma once

#include "grid_mgr.h"

namespace GridManager {

enum SpecialSyms {
    SS_BS   = 0x101,
    SS_Ent  = 0x103,
    SS_Space= 0x106,
    SS_Lang = 0x109
};

extern Geometry keyb_combo_eng, keyb_combo_rus, keyb_eng, keyb_rus, keyb_digits;

enum KeybOption {
    KBO_Left    = 0x0001,   // Place input line in left part of reserved area (if both parts defined)
    KBO_UseBot  = 0x0002,   // Bottom part of Reserved area to place Keyboar Input line
};
struct KeybBoxDef {
    BoxDef box_def;
    uint16_t kb_bg_text_color;
    uint16_t kb_input_line_shift;
    uint16_t kb_options; // KeybOption bitset
};

class Keyboard : public Grid {
    const Geometry* geoms[2];
    const KeybBoxDef& kb_def;
    int cur_geom = 0;
    char sym_buf[5]; // Up to 4 symbols in Special keyboard keys supported

    // Keyboard part
    char kb_buffer[51]; // Our screen can hold only up to 50 symbols (no gaps, no margins, no box - just raw symbols)
    int kb_pos=0;       // Position in KB buffer
    bool kb_flag = false; // Flag 'KB cursor is shown'
    int16_t kb_x, kb_y; // x & y of Keyboard input line start
    int16_t kb_symbols; // Total symbols in Keyboard line

public:
    Keyboard(const KeybBoxDef& b, const Geometry& g1, const Geometry& g2, int strip_lines=0) : Grid(b.box_def, g1, strip_lines), kb_def(b)
    {
        geoms[0] = &g1;
        geoms[1] = &g2;
    }

    void switch_lang(LCD& lcd)
    {
        swap_geometry(geoms[cur_geom^=1]);
        printf("Geom=%d\n",cur_geom);
        update(lcd);
    }

    virtual const char* get_text_dos(const MiniCell&) override;

    // Keyboard interface
    void kb_activate(LCD& lcd); // Draw keyboard input line and cursor

    /*
        Call method 'kb_process' on touch, after call to 'get_touch'.
        Got ID from get_touch and returns flag 'Enter'

        How to call:

        Action act = activity.get_action();
        if (act.type == AT_TouchDown)
        {
            int id = this->get_touch(act.touch.x, act.touch.y);
            if (id == -1) <some>; // Not keyboard event
            else if (this->kb_process(id)) {return this->kb_get_string();}
        }
    */
    bool kb_process(LCD& lcd, int id);
    const char* kb_get_string() {kb_buffer[kb_pos]=0; return kb_buffer;} // Returns image of string entered so far
    void kb_animate(LCD& lcd) {kb_set_cursor(lcd, !kb_flag);} // Call to animate keyboard cursor
    void kb_set_cursor(LCD& lcd, bool turn_on); // Turn on/off cursor
};


} // namespace GridManager
