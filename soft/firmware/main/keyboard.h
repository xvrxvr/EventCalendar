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

class Keyboard : public Grid {
    const Geometry* geoms[2];
    int cur_geom = 0;
    char sym_buf[5]; // Up to 4 symbols in Special keyboard keys supported
public:
    Keyboard(const BoxDef& b, const Geometry& g1, const Geometry& g2, int strip_lines=0) : Grid(b, g1, strip_lines)
    {
        geoms[0] = &g1;
        geoms[1] = &g2;
    }

    void switch_lang(LCD& lcd)
    {
        swap_geometry(geoms[cur_geom^=1]);
        update(lcd);
    }

    virtual const char* get_text_dos(const MiniCell&) override;
};


} // namespace GridManager
