#include "common.h"
#include "grid_mgr.h"
#include "keyboard.h"
#include "activity.h"
#include "challenge_list.h"
#include "icons.h"

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

struct KbDefByType {
    Geometry* kb1;
    Geometry* kb2=NULL;
    int strip_lines=0;
};

static KbDefByType kb_defs_by_type[8] = {
    {&keyb_combo_rus, &keyb_combo_eng}, // Empty KB ???
    {&keyb_eng, NULL, 1},               // GO_KbEnglish
    {&keyb_rus, NULL, 1},               // GO_KbRussian
    {&keyb_combo_eng, &keyb_combo_rus, 1}, // GO_KbEnglish+GO_KbRussian
    {&keyb_digits, NULL},               // GO_KbNumbers
    {&keyb_eng, NULL},                  // GO_KbNumbers+GO_KbEnglish
    {&keyb_rus, NULL},                  // GO_KbNumbers+GO_KbRussian
    {&keyb_combo_eng, &keyb_combo_rus} // GO_KbNumbers+GO_KbEnglish+GO_KbRussian
};

Keyboard::Keyboard(const KeybBoxDef& b, int kb_type) : Grid(b.box_def, *kb_defs_by_type[kb_type&7].kb1, kb_defs_by_type[kb_type&7].strip_lines), kb_def(b)
{
    geoms[0] = kb_defs_by_type[kb_type&7].kb1;
    geoms[1] = kb_defs_by_type[kb_type&7].kb2;
    if (!geoms[1]) geoms[1] = geoms[0];
}

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
    wipe_kb_box(lcd);
    kb_set_cursor(lcd, true);
}

void Keyboard::wipe_kb_box(LCD& lcd)
{
    lcd.WRect(kb_x, kb_y, kb_symbols*8, 16, kb_def.kb_bg_text_color);
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

// Writes message into input string. Wait 5 seconds and wipe out (input buffer and cursor also cleared). Method blocked inside for 5 seconds.
// Message can includes control codes (\1 to \9) to switch color. \1 denotes original color of input line, \2 and so on defined in 'colors' array
void Keyboard::message_utf8(LCD& lcd, const char* msg, uint16_t* colors)
{
    Prn b(msg);

    utf8_to_dos(b.c_str());
    kb_set_cursor(lcd, false);
    wipe_kb_box(lcd);

    lcd.set_bg(kb_def.kb_bg_text_color);
    lcd.set_fg(box_defs[0].fg_color);

    int x = kb_x;
    int total = 0;

    for(const char* s = b.c_str(); *s && total < kb_symbols; ++s)
    {
        if (uint8_t(*s) <= 9)
        {
            if (*s == 1) lcd.set_fg(box_defs[0].fg_color);
            else lcd.set_fg(colors[*s-2]);
        }
        else
        {
            lcd.text(s, x, kb_y, 1);
            ++total;
            x+=8;
        }
    }

    vTaskDelay(s2ticks(SC_KbMsgShow));
    wipe_kb_box(lcd);
    kb_pos = 0;
}

#define Lcd() Activity::LCDAccess(NULL).access()

// <red>, <green>
static uint16_t colors[] = {0xF904, 0x27E8};

int Keyboard::default_kb_process(std::function<bool()> test_func)
{
    Activity act(AT_TouchDown|AT_WatchDog); // Add timeout! (Make separate microtick support)
    act.setup_watchdog_ticks(50);

    bool help_active = false;
    int error_count = 0;    

    kb_activate(Lcd());
    for(;;)
    {
        auto a = act.get_action();
        if (a.type == AT_TouchDown)
        {
            int id = get_touch(a.touch.x, a.touch.y).id;
            if (id == -1)
            {
                if (help_active && a.touch.y < 32 && a.touch.x > RES_X-32) return 0;
                continue;
            } 
            if (kb_process(Lcd(), id))
            {
                if (test_func())
                {
                    message_utf8(Lcd(), "\3Правильно", colors);
                    return CR_Ok;
                }
                else
                {
                    message_utf8(Lcd(), "\2Не правильно\3 Ещё раз ...", colors);
                    ++error_count;
                    if (error_count == 3)
                    {
                        help_active = true;
                        Lcd().icon32x32(RES_X-32, 0, help_icon, 0x27E8);
                    }
                }
            }
        } else
        if (a.type == AT_WatchDog) kb_animate(Lcd());
    }

}


}
