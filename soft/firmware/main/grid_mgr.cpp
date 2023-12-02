#include "common.h"

#include "grid_mgr.h"

namespace GridManager {

void Row::add_string(const char* s)
{
    Prn b; b.strcpy(s);
    utf8_to_dos(b.c_str());
    for(uint8_t* p =(uint8_t*)b.c_str(); *p ; ++p) (*this)(*p);
}


Rect Grid::shrink(const Rect& out_box, const TextBoxDraw::TextGlobalDefinition& tdef)
{
    Rect result = out_box;

    auto res_space = tdef.reserved_space();
    result.width -= res_space.first;
    result.height -= res_space.second;

    auto shift = tdef.min_dist_to_text();
    result.x += shift.first;
    result.y += shift.second;

    return result;
}

// Performs initial Geometry evaluation.
// Called at beginig of EVERY interface function, but really executed only once
void Grid::initial_geom_eval()
{
    if (!cells.empty()) return;
    col_count = geom->row_size();
    row_count = geom->total_rows() - strip_lines;
    cells.resize(col_count*row_count);

    int cell_w = 0;

    for(int r=0; r<row_count; ++r)
    {
        for(int c=0, cc=0; c<col_count; ++cc)
        {
            auto& C = cell(r, c);
            (MiniCell&)C = geom->cell(r+strip_lines, cc);
            if (C.col_span < 1) C.col_span = 1;
            C.updated = -1;
            C.box_index = 1;
            if (!bdef.cell_width)
            {
                const char* text = get_text_dos(C);
                if (text) cell_w = std::max<int>(cell_w, (strlen(text)*8 + C.col_span-1) / C.col_span);
            }
            c += C.col_span;
        }
    }

    bdef.cell_width = std::max<int16_t>(bdef.cell_width, cell_w);

    auto res_space = box_defs[1].reserved_space();

    int cell_outer_w = bdef.cell_width + res_space.first;
    int cell_outer_h = bdef.cell_height + res_space.second;

    int y = 0;
    for(int r=0; r<row_count; ++r)
    {
        int x = 0;
        for(int c=0; c<col_count;)
        {
            auto& C = cell(r, c);
            C.box.width = cell_outer_w;
            C.box.height = cell_outer_h;
            C.box.x = x;
            C.box.y = y;
            x += (cell_outer_w + box_defs[1].marging_h) * C.col_span;
            c += C.col_span;
        }
        y += cell_outer_h + box_defs[1].marging_v;
    }
    grid_bounds.width = (cell_outer_w + box_defs[1].marging_h) * col_count - box_defs[1].marging_h;
    grid_bounds.height = (cell_outer_h + box_defs[1].marging_v) * row_count - box_defs[1].marging_v;
}

// Prepare and draw Grid inside supplied coordinates
void Grid::set_coord(LCD& lcd, const Rect& rect)
{
    int my_internal_w = grid_bounds.width + bdef.reserve_left;
    int my_internal_h = grid_bounds.height + bdef.reserve_top;

    auto reserved = box_defs[0].reserved_space();

    int ext_w = my_internal_w + reserved.first;
    int ext_h = my_internal_h + reserved.second;

    assert(ext_w <= rect.width);
    assert(ext_h <= rect.height);

    bounds.width = ext_w;
    bounds.height = ext_h;

    bounds.x = (rect.width - ext_w) / 2 + rect.x;
    bounds.y = (rect.height - ext_h) / 2 + rect.y;

    auto box_shift = box_defs[0].min_dist_to_text();
    grid_bounds.x = bounds.x + box_shift.first;
    grid_bounds.y = bounds.y + box_shift.second;

    box_defs[0].draw_box(lcd, bounds.x, bounds.y, bounds.width, bounds.height, true);
    update(lcd);
}

Grid::TouchCell Grid::get_touch(int x, int y) const
{
    TouchCell result;

    x-=grid_bounds.x;
    y-=grid_bounds.y;

    if (x < 0 || y < 0) return result;

    int r;
    for(r=0; r<row_count;++r)
    {
        const auto& c =cell(r, 0);
        if (c.box.y > y) return result;
        if (c.box.y <= y && c.box.y + c.box.height > y) break;
    }
    if (r==row_count) return result;
    for(int c=0; c < col_count;)
    {
        const auto& C = cell(r, c);
        if (C.box.x > x) return result;
        if (C.box.x <= x && C.box.x + C.box.width > x)
        {
            result.row = r;
            result.col = c;
            result.id = C.id;
            return result;
        }
        c += C.col_span;
    }
    return result;
}

void Grid::swap_geometry(const Geometry* new_geom)
{
    if (geom == new_geom) return;
    assert(new_geom->row_size() == col_count);
    assert(new_geom->total_rows() - strip_lines == row_count);

    for(int r=0; r<row_count; ++r)
    {
        for(int c=0, cc=0; c<col_count; ++cc)
        {
            auto& C = cell(r, c);
            auto const &CC = geom->cell(r+strip_lines, cc);
            assert(C.col_span == CC.col_span);
            if (C.id != CC.id)
            {
                if ( (C.id == -1) != (CC.id == -1) ) // We turn on/off cell
                {
                    C.updated |= UI_Box|UI_Text;
                }
                else // Text may be changed
                {
                    auto t1 = get_text_dos(C);
                    auto t2 = get_text_dos(CC);
                    if (t1 != t2)
                    {
                        if ( (t1 == NULL) != (t2 == NULL) ) // Turn on/off text
                        {
                            C.updated |= t2 ? UI_Text : UI_Box|UI_Text; // If turn on (t2) - just text update will be enough, otherwise - Box need to be redrawn
                        }
                        else if (strcmp(t1, t2)) // Text changed - update
                        {
                            assert(strlen(t1) == strlen(t2)); // We do not support text length change (for now)
                            C.updated |= UI_Text;
                        }
                    }
                }
                C.id = CC.id;
            }
            c += C.col_span;
        }
    }
    geom = new_geom;
}

// Draw one cell
void Grid::draw_cell(LCD& lcd, const Cell& cell, int update, int dx, int dy)
{
    const char* text = NULL;
    if (update & UI_Text) text = get_text_dos(cell);

    int bx = cell.box.x+dx;
    int by = cell.box.y+dy;

    if (update & UI_Box)
    {
        if (cell.id != -1 && cell.box_index) box_defs[cell.box_index].draw_box(lcd, bx, by, cell.box.width, cell.box.height, false); 
        else draw_spacer(lcd, Rect{.x=bx, .y=by, .width=cell.box.width, .height= cell.box.height});
    }
    if (text)
    {
        auto box_reserved = box_defs[cell.box_index].reserved_space();
        auto box_shift = box_defs[cell.box_index].min_dist_to_text();
        bx += (cell.box.width - box_reserved.first - strlen(text)*8) / 2 + box_shift.first;
        by += (cell.box.height - box_reserved.second - 16) / 2 + box_shift.second;
        lcd.text(text, bx, by);
    }
}

// Draw spacer
void Grid::draw_spacer(LCD& lcd, const Rect& b1, const Rect& b2, int dx, int dy)
{
    Rect to_fill = b1;
    if (b1.x == b2.x) // Vertical
    {
        to_fill.y += b1.height;
        to_fill.height = b2.y - to_fill.y;
    }
    else // Horizontal
    {
        to_fill.x += b1.width;
        to_fill.width = b2.x - to_fill.x;
    }
    to_fill.x += dx; to_fill.y += dy;
    draw_spacer(lcd, to_fill);
}

void Grid::draw_spacer(LCD& lcd, const Rect& to_fill)
{
    lcd.WRect(to_fill.x, to_fill.y, to_fill.width, to_fill.height, box_defs[0].bg_color);
}


// Draw diff spacer for floating rect
void Grid::draw_float_spacer(LCD& lcd, int row, int col, int row_count, int col_count, int dx, int dy)
{
    Rect to_fill;
    const auto &c1 = cell(row, col);
    const auto &c2 = cell(row+row_count-1, col+col_count-1);

    to_fill.x = c1.box.x + prev_dx;
    to_fill.y = c1.box.y + prev_dy;
    to_fill.width = c2.box.x + c2.box.width - c1.box.x;
    to_fill.height = c2.box.y + c2.box.height - c1.box.y;

    if (dx > prev_dx) to_fill.width = dx-prev_dx; else
    if (dx < prev_dx) {auto d = prev_dx - dx; to_fill.x += to_fill.width - d; to_fill.width = d;} else
    if (dy > prev_dy) to_fill.height = dy-prev_dy; else
    if (dy < prev_dy) {auto d = prev_dy - dy; to_fill.y += to_fill.height - d; to_fill.height = d;} 
    else return;

    draw_spacer(lcd, to_fill);
}


// Draw subgrid (row/col by row_count/col_count cells) with shift dx/dy from its original position.
// Keep track of dx/dy changes to clear possible tails of previous subgrid images
// History cleared by 'update' call
void Grid::draw_float(LCD& lcd, int row, int col, int row_count, int col_count, int dx, int dy)
{
    dx += grid_bounds.x; dy += grid_bounds.y;
    draw_float_spacer(lcd, row, col, row_count, col_count, dx, dy);
    prev_dx = dx; prev_dy = dy;

    for(int r=0; r<row_count; ++r)
        for(int c=0; c<col_count;)
        {
            auto& C = cell(r+row, c+col);
            lcd.set_fg(box_defs[C.box_index].fg_color);
            lcd.set_bg(box_defs[C.box_index].bg_color);
            draw_cell(lcd, C, -1, dx, dy);
            if (c+1 < col_count) draw_spacer(lcd, C.box, cell(c+1+col, r+row).box, dx, dy);
            if (r+1 < row_count) draw_spacer(lcd, C.box, cell(c+col, r+1+row).box, dx, dy);
            c += C.col_span;
        }
}

// Draw grid on screen
void Grid::update(LCD& lcd, bool with_spacers)
{
    prev_dx = grid_bounds.x;
    prev_dy = grid_bounds.y;

    for(int r=0; r<row_count; ++r)
        for(int c=0; c<col_count;)
        {
            auto& C = cell(r, c);
            lcd.set_fg(box_defs[C.box_index].fg_color);
            lcd.set_bg(box_defs[C.box_index].bg_color);
            if (C.updated)
            {
                draw_cell(lcd, C, C.updated, grid_bounds.x, grid_bounds.y);
                C.updated = 0;
                if (with_spacers)
                {
                    if (c+1 < col_count && cell(c+1, r).updated) draw_spacer(lcd, C.box, cell(c+1, r).box, grid_bounds.x, grid_bounds.y);
                    if (r+1 < row_count && cell(c, r+1).updated) draw_spacer(lcd, C.box, cell(c, r+1).box, grid_bounds.x, grid_bounds.y);
                }
            }
            c += C.col_span;
        }
}

} // namespace GridManager
