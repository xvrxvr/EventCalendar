#include "common.h"
#include "buttons.h"
#include "activity.h"

#define GLCD Activity::LCDAccess(NULL).access()

TextBoxDraw::TextsParser& Buttons::pars_init(TextBoxDraw::TextsParser& tp)
{
    tp.parse_text(buf.c_str());
    tp.eval_mbox_sizes(tp.MB_ALL, box_w, box_h);
    return tp;
}

#define PARS(setup)  TextBoxDraw::TextsParser tp(setup); pars_init(tp)

void Buttons::draw(int x, int y, int w, int h)
{
    box_x = x; box_y = y; box_w = w; box_h = h;
    if (is_scroll_active()) options |= BO_ShowDnArrow;
    fill_cdefs_indexes();

    PARS(bs.not_pressed).draw_selection_of_boxes(GLCD, cdefs, total_boxes(), 0, x, y, w, h);
    draw_gray_border();
    int idx = index2screen(cur_pressed);
    if (idx != -1) draw_one(idx, true);
}

void Buttons::fill_cdefs_indexes()
{
    int idx=0;
    int total = MAX_ROWS;
    if (options & BO_ShowUpArrow) {cdefs[idx++].index = 0; --total;}
    if (options & BO_ShowDnArrow) --total;
    for(int i=0; i<total; ++i) cdefs[idx++].index = i+cur_line+2;
    if (options & BO_ShowDnArrow) cdefs[idx++].index = 1;
}

void Buttons::scroll_to(int start_index)
{
    if (start_index == cur_line) return;
    options &= ~(BO_ShowUpArrow|BO_ShowDnArrow);
    if (is_scroll_active())
    {
        int total = MAX_ROWS;
        if (start_index) {options |= BO_ShowUpArrow; --total;}
        if (buttons_indexes.size() - start_index > total) options |= BO_ShowDnArrow;
    }
    int idx = index2screen(cur_pressed);
    if (idx != -1) draw_one(idx, false);
    cur_line = start_index;
    fill_cdefs_indexes();
    PARS(bs.not_pressed).draw_selection_of_boxes(GLCD, cdefs, total_boxes(), 0, box_x, box_y, box_w, box_h);
    idx = index2screen(cur_pressed);
    if (idx != -1) draw_one(idx, true);
}

void Buttons::draw_gray_border()
{
    const auto bsize = bs.gray_area_width;
    const auto bcolor = bs.gray_area_color;
    Activity::LCDAccess lcda(NULL);
    auto& lcd = lcda.access();

    int total = total_boxes();
    for(int idx=0; idx<total; ++idx)
    {
        auto& C = cdefs[idx];
        lcd.WRect(C.x-bsize, C.y-bsize, C.width+2*bsize, bsize, bcolor); // Top bar
        lcd.WRect(C.x-bsize, C.y+C.height, C.width+2*bsize, bsize, bcolor); // Down bar
        lcd.WRect(C.x-bsize, C.y, bsize, C.height, bcolor); // Left bar
        lcd.WRect(C.x+C.width, C.y, bsize, C.height, bcolor); // Right bar
    }    
}

void Buttons::draw_one(int screen_index, bool pressed)
{
    Activity::LCDAccess lcda(NULL);
    auto& C = cdefs[screen_index];
    auto& lcd = lcda.access();
    TextBoxDraw::TextGlobalDefinition p_setup = get_pressed_setup();
    int delta = pressed ? bs.press_shift : 0;
    PARS(pressed ? p_setup : bs.not_pressed).draw_one_box_of_selection_of_boxes(lcd, C, delta, delta);

    if (pressed) // Draw gray top left corner
    {
        const auto bcolor = bs.gray_area_color;
        lcd.WRect(C.x, C.y, C.width, delta, bcolor); // Top bar
        lcd.WRect(C.x, C.y, delta, C.height, bcolor); // Left bar
    }
}

int Buttons::press(int x, int y)
{
    int total = total_boxes();
    for(int idx=0; idx<total; ++idx)
    {
        auto& c = cdefs[idx];
        if (x >=c.x && x < c.x+c.width && y >= c.y && y < c.y+c.height) return press_imp(idx);
    }
    return PressNo;
}

int Buttons::press_imp(int screen_row)
{
    int delta = 0 ;
    // Move from/to 1st line (no Up Arrow) right to 3d to ensure visual move (skipping just 1 line will not move buttons - top selection will be just replaced by Up Arrow)
    if (screen_row == 0 && (options & BO_ShowUpArrow)) delta = cur_line <= 2 ? -cur_line : -1; else
    if (screen_row == MAX_ROWS-1 && (options & BO_ShowDnArrow)) delta = cur_line == 0 ? 2 : 1;
    if (delta)
    {
        visual_feedback(screen_row);
        scroll_to(cur_line+delta);
        return PressInternal;
    }
    int line_index = screen_row + cur_line;
    if (options & BO_ShowUpArrow) --line_index;
    if (options & BO_RadioButton)
    {
        if (cur_pressed == line_index) return PressInternal;
        int old = index2screen(cur_pressed);
        if (old != -1) draw_one(old, false);
        draw_one(screen_row, true);
    }
    else
    {
        visual_feedback(screen_row);
    }
    cur_pressed = line_index;
    return get_currently_pressed();
}
///////////
ButtonsSetup::ButtonsSetup()
{
    //not_pressed.setup("");
}

ButtonsSetup global_buttons_setup;
