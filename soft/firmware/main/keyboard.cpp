#include "common.h"
#include "grid_mgr.h"
#include "keyboard.h"

namespace GridManager {

Geometry keyb_combo_eng( Rows() 
    << Row("1234567890")
    << Row("QWERTYUIOP()")(SS_BS,2)
    << Row("ASDFGHJKL:@")(SS_Ent, 3)
    << Row("ZXCVBNM,.?/")(SS_Space, 2)(SS_Lang, 1)
);

Geometry keyb_combo_rus(Rows() 
    << Row("1234567890")
    << Row("ЙЦУКЕНГШЩЗХЪ")(SS_BS,2)
    << Row("ФЫВАПРОЛДЖЭ")(SS_Ent, 3)
    << Row("ЯЧСМИТЬБЮЁ.")(SS_Space, 2)(SS_Lang, 1)
);

Geometry keyb_eng( Rows() 
    << Row("1234567890")
    << Row("QWERTYUIOP")(SS_BS,2)
    << Row("ASDFGHJKL")(SS_Ent, 3)
    << Row("ZXCVBNM.")(SS_Space, 4)
);

Geometry keyb_rus(Rows() 
    << Row("1234567890")
    << Row("ЙЦУКЕНГШЩЗХЪ")(SS_BS,2)
    << Row("ФЫВАПРОЛДЖЭ")(SS_Ent, 3)
    << Row("ЯЧСМИТЬБЮЁ.")(SS_Space, 3)
);

Geometry keyb_digits(Rows() 
    << Row("01234")(SS_BS, 2)
    << Row("56789")(SS_Ent, 2)
);

const char* Keyboard::get_text_dos(const MiniCell& cell)
{
    int id = cell.id;
    if (id == -1) return NULL;
    if (id < 0x100) {sym_buf[0] = id; sym_buf[1] = 0; return sym_buf;}
    sym_buf[0] = id & 0xFF;
    if (cell.col_span > 1)
    {
        assert(cell.col_span <= std::size(sym_buf)-1);
        if (cell.col_span > 2) memset(sym_buf+1, (id & 0xFF) + 2, cell.col_span-2);
        sym_buf[cell.col_span-1] = (id & 0xFF) + 1;
    }
    sym_buf[cell.col_span] = 0;
    return sym_buf;
}


void Keyboard::kb_activate(LCD& lcd) // Draw keyboard input line and cursor
{
    Rect area = get_reserved(kb_def.box_def.reserve_top && !(kb_def.kb_options & KBO_Left));
    if (kb_def.kb_options & KBO_UseBot) area.y += area.height - 8 - kb_def.kb_input_line_shift; // use bottom
    else area.y += kb_def.kb_input_line_shift;
    area.height = 8;    
    kb_x = area.x; kb_y = area.y; kb_symbols = area.width/8;
    kb_x += (area.width - kb_symbols*8) >> 1;
    lcd.WRect(kb_x, kb_y, kb_symbols*8, 16, kb_def.kb_bg_text_color);
    kb_set_cursor(lcd, true);
}

bool Keyboard::kb_process(LCD& lcd, int id)
{
    bool advance = true;
    switch(id)
    {
        case SS_BS: 
            if (!kb_pos) return false;
            if (kb_flag) {kb_set_cursor(lcd, false); kb_flag=true;}
            --kb_pos;
            advance = false;
            id = ' ';
            break;
        case SS_Ent: return true;
        case SS_Space: id = ' '; break;
        case SS_Lang: switch_lang(lcd); return false;
        case -1: return false;
        default: break;
    }
    if (kb_pos >= kb_symbols) return false;
    kb_buffer[kb_pos] = id;
    lcd.set_fg(box_defs[0].fg_color);
    lcd.set_bg(kb_def.kb_bg_text_color);
    lcd.text((char*)&id, kb_x + kb_pos*8, kb_y, 1);
    if (advance) ++kb_pos;
    if (kb_flag)
    {
        kb_flag = false;
        kb_set_cursor(lcd, true);
    }
    return false;
}

void Keyboard::kb_set_cursor(LCD& lcd, bool turn_on) // Turn on/off cursor
{
    if (turn_on == kb_flag) return;
    kb_flag = turn_on;
    if (kb_pos >= kb_symbols) return;
    lcd.set_fg(box_defs[0].fg_color);
    lcd.set_bg(kb_def.kb_bg_text_color);
    char sym = kb_flag ? '_' : ' ';
    lcd.text(&sym, kb_x + kb_pos*8, kb_y, 1);
}

}
