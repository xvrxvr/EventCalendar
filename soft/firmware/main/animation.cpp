#include "common.h"
#include "animation.h"
#include "hadrware.h"
#include "text_draw.h"
#include "box_draw.h"
#include "ILI9327_Shield.h"
#include "activity.h"

static void operator<< (std::unique_ptr<char[]>& dst, Prn& src)
{
    char* b = new char[src.length()+1];
    strcpy(b, src.c_str());
    dst.reset(b);
}

std::pair<int16_t, int16_t> SizeDef::get_size(bool double_size_letter, int title_length) const
{
    int16_t h = (1+max_text_lines) * (double_size_letter ? 32 : 16) + SC_AminPanelTitleGap + 32 * max_icons_lines;
    int16_t w = std::max(title_length, max_text_length) * (double_size_letter ? 16 : 8);
    w = std::max<int16_t>(w, 32 * max_icons_count);
    return {w, h};
}

AnimatedPannel::AnimatedPannel(const char* title_utf8, const SizeDef& size, const TextBoxDraw::TextGlobalDefinition* box_def) : 
    bdef(box_def ? *box_def : TextBoxDraw::default_text_global_definition)
{
    Prn b; b.strcpy(title_utf8); utf8_to_dos(b.c_str()); title << b;
    auto len = strlen(b.c_str());
    auto reserve_box_sz = bdef.reserved_space();
    use_doube_size_letters = true;
    for(;;)
    {
        auto my_sz = size.get_size(use_doube_size_letters, len);
        box_width = my_sz.first + reserve_box_sz.first;
        box_height = my_sz.second + reserve_box_sz.second;
        if (box_width < RES_X - 2*bdef.marging_h && box_height < RES_Y - 2*bdef.marging_v) break;
        if (!use_doube_size_letters) abort();
        use_doube_size_letters = false;
    }
}

void AnimatedPannel::body_reset()
{
    prev_total_lines = total_lines;
    total_lines = 0;
    start_line_to_redraw = 0;
}

void AnimatedPannel::add_text_utf8(const char* msg, ...)
{
    assert(total_lines < MaxLines);

    va_list l;
    va_start(l, msg);
    Prn b; b.vprintf(msg, l);
    va_end(l);
    utf8_to_dos(b.c_str());

    if (prev_total_lines <= total_lines && lines[total_lines].text && strcmp(lines[total_lines].text.get(), b.c_str())==0) // The same - skip
    {
        if (start_line_to_redraw == total_lines) ++start_line_to_redraw;
        ++total_lines;
        return;
    }
    lines[total_lines].text << b;
    ++total_lines;
}

void AnimatedPannel::add_icons(uint32_t* icon_, int count, uint16_t color)
{
    assert(total_lines < MaxLines);
    bool is_same = (icon_ == icon && count == icon_count && color == icon_color);
    if (!is_same) {icon = icon_; icon_count = count; icon_color = color;}

    if (icons_line == total_lines && is_same)
    {
        if (start_line_to_redraw == total_lines) ++start_line_to_redraw;
        ++total_lines;
        return;
    }

    lines[total_lines].text.reset();
    icons_line = total_lines;
    ++total_lines;
}

int AnimatedPannel::lcd_text(LCD& lcd, const char* msg, int x, int y)
{
    if (use_doube_size_letters) lcd.text2(msg, x, y);
    else lcd.text(msg, x, y);
    return strlen(msg) * sym_w();
}

void AnimatedPannel::body_draw(LCD& lcd)
{
    lcd.set_bg(bdef.bg_color);
    lcd.set_fg(bdef.fg_color);

    if (first_draw)
    {
        first_draw = false;
        start_line_to_redraw = 0;
        int16_t box_x = (RES_X - box_width) >> 1;
        int16_t box_y = (RES_Y - box_height) >> 1;
        auto sh = bdef.min_dist_to_text();
        text_x = box_x + sh.first;
        text_y = box_y + sh.second;
        text_w = box_width - bdef.reserved_space().first;

        bdef.draw_box(lcd, box_x, box_y, box_width, box_height, true);

        int title_len = strlen(title.get());
        int dx = (text_w - title_len * sym_w()) >> 1;
        lcd_text(lcd, title.get(), text_x + dx, text_y);
    }
    int start_y = text_y + sym_h() + SC_AminPanelTitleGap;
    for(int line=0; line<total_lines; ++line)
    {
        auto const& L = lines[line];

        if (line >= start_line_to_redraw)
        {
            if (L.text) draw_text(lcd, L.text.get(), start_y); else
            {
                main_anim.abort(); 
                oob_anim.abort();
                icon_y = start_y;
                draw_icons(lcd);
            }
        }
        start_y += L.text ? sym_h() : 32;
    }
}

void AnimatedPannel::draw_text(LCD& lcd, const char* msg, int y)
{    
    int w = lcd_text(lcd, msg, text_x, y);
    int h = sym_h();
    lcd.WRect(text_x + w, y, text_w - w, h, bdef.bg_color);
}

void AnimatedPannel::draw_icons(LCD& lcd)
{
    int x = text_x;
    for(int i=0; i< icon_count; ++i)
    {
        lcd.icon32x32(x, icon_y, icon, icon_color);
        x += 32;
    }
    lcd.WRect(x, icon_y, text_w - 32*icon_count, 32, bdef.bg_color);
}

AnimatedPannel& AnimatedPannel::animate_icon(int icon_index, const AnimationSetup& setup, bool override)
{
    (override?oob_anim:main_anim).assign(setup, icon_index);
    return *this;
}

void AnimatedPannel::tick(LCD& lcd, bool step)
{
    if (oob_anim.is_active()) animate(lcd, oob_anim, step);
    if (main_anim.is_active() && !(oob_anim.is_active() && main_anim.icon_index == oob_anim.icon_index)) animate(lcd, main_anim, step);
}

void AnimatedPannel::animate(LCD& lcd, Animation& a, bool do_tick)
{
    uint16_t color=0;
    switch(a.anim.type)
    {
        case AT_None: color = a.anim.color_from; a.abort(); break;
        case AT_One:        
            if (a.tick >= a.anim.length) {color = a.anim.color_to; a.abort();}
            else color=a.color(true);
            break;
        case AT_Pulse: // Animate <color_from> -> <color_to> -> <color_from> one time
            if (a.tick >= a.anim.length) // Switch stage
            {
                ++a.stage;
                a.tick = 0;
            }
            switch(a.stage)
            {
                case 0: color = a.color(true); break;
                case 1: color = a.color(false); break;
                default: color = a.anim.color_from; a.abort(); break;
            }
            break;
        case AT_Periodic:
            if (a.tick >= a.anim.length) a.tick = 0;
            color = a.color(true); 
            break;
        case AT_Triange: // Animate <color_from> -> <color_to> -> <color_from> many times
            if (a.tick >= a.anim.length) // Switch stage
            {
                ++a.stage &= 1;
                a.tick = 0;
            }
            color = a.color(a.stage == 0);
            break;
    }
    lcd.icon32x32(text_x + a.icon_index*32, icon_y, icon, color);
    if (do_tick) a.tick++;
}

uint16_t AnimatedPannel::Animation::color(bool raising_slope)
{
    const int t = raising_slope ? tick : anim.length - tick - 1;
    const int l = anim.length-1;

    auto enc = [t,l] (int from, int to) {return from + (to-from) * t / l; };

#define SEL(who, sh, width) ((anim.color_##who >> sh) & (bit(width)-1))
#define E(sh, width) enc( SEL(from, sh, width), SEL(to, sh, width)) & (bit(width)-1)
    int r = E(11,5);
    int g = E(5, 6);
    int b = E(0, 5);
#undef E
#undef SEL
    return (r<<11) | (g<<5) | b;
}

void AnimatedPannel::body_draw()
{
    body_draw(Activity::LCDAccess(NULL).access());
}

void AnimatedPannel::tick(bool step)
{
    tick(Activity::LCDAccess(NULL).access(), step);
}
