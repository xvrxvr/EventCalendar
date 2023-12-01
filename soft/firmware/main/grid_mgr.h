#pragma once

#include "text_draw.h"

namespace GridManager {


enum UpdateItem {
    UI_Text  = 0x01,
    UI_Box   = 0x02
};

struct MiniCell {
    int id=-1;
    int col_span=1;
};

struct Row {
    std::vector<MiniCell> cells;
    int total=0;

    void add_string(const char*);
public:
    Row(const char* s = NULL) {if (s) add_string(s);}

    Row& operator () (const char* s) {add_string(s); return *this;}
    Row& operator () (int id, int col_span=1) {assert(col_span>=1); cells.push_back(MiniCell{id, col_span}); total+=col_span; return *this;}

    size_t row_size() const {return total;}

    MiniCell operator() (int col) const {return col < cells.size() ? cells[col] : MiniCell{};}
};

class Rows {
    std::vector<Row> rows;
public:
    Rows& operator << (const Row&& r) {rows.push_back(r); return *this;}

    size_t row_size() const 
    {
        size_t result = 0;
        for(const auto& r: rows) result = std::max(result, r.row_size());
        return result;
    }
    size_t total_rows() const {return rows.size();}

    MiniCell operator() (int row, int col) const {return rows[row](col);}
};

class Geometry {
    Rows rows;
public:
    Geometry(const Rows&& r) : rows(r) {}

    size_t row_size() const  {return rows.row_size();}
    size_t total_rows() const {return rows.total_rows();}
    MiniCell cell(int row, int col) const {return rows(row, col);}
};

struct Rect {
    int x=-1;
    int y=-1;
    int width=0;
    int height=0;
};

struct BoxDef {
    TextBoxDraw::TextGlobalDefinition outer_box_def, cell_box_def;
    int16_t reserve_top = 0; // -1 for reserve all availabe space
    int16_t reserve_left = 0; // -1 for reserve all availabe space
    int16_t cell_width = 0; // Set to >0 value to define internal cell width. By default defined by cell contents
    int16_t cell_height = 16; // Define internal cell height.

    constexpr BoxDef res_top(int16_t val) const {BoxDef res(*this); res.reserve_top = val; return res;}
    constexpr BoxDef res_left(int16_t val) const {BoxDef res(*this); res.reserve_left = val; return res;}
    constexpr BoxDef c_width(int16_t val) const {BoxDef res(*this); res.cell_width = val; return res;}
    constexpr BoxDef c_height(int16_t val) const {BoxDef res(*this); res.cell_height = val; return res;}
    BoxDef outer_b(const char* v) const {BoxDef res(*this); res.outer_box_def.setup(v); return res;}
    BoxDef cell_b(const char* v) const {BoxDef res(*this); res.cell_box_def.setup(v); return res;}
};

class Grid {
    struct Cell : public MiniCell {
        Rect box;      // x,y from top/left of internal Grid area (grid_bounds). box itself represents Box for this cell (not internal contents)
        int updated=0; // bitset of UpdateItem
    };
    BoxDef bdef;
    const Geometry* geom;
    int strip_lines = 0;

    int col_count=0;
    int row_count=0;
    std::vector<Cell> cells;

    Rect bounds; // External box coordinates. Set by 'set_coord' method call
    Rect grid_bounds; // Rect of grid itself. width & haight depends on grid only, but x & y will be set by 'set_coord' method call

    int16_t prev_dx = 0;  // Previous dx/dy in 'draw_float' call
    int16_t prev_dy = 0;

    Cell& cell(int row, int col) {return cells[col + row*col_count];}

    const Cell& cell(int row, int col) const {return cells[col + row*col_count];}

    static Rect shrink(const Rect& out_box, const TextBoxDraw::TextGlobalDefinition&);

    // Performs initial Geometry evaluation.
    // Called at beginig of EVERY interface function, but really executed only once
    void initial_geom_eval();

    // Draw one cell
    void draw_cell(LCD& lcd, const Cell& cell, int update, int dx, int dy);

    // Draw spacer
    void draw_spacer(LCD& lcd, const Rect&);

    // Draw spacer between boxes
    void draw_spacer(LCD& lcd, const Rect& b1, const Rect& b2, int dx, int dy);

    // Draw diff spacer for floating rect
    void draw_float_spacer(LCD& lcd, int row, int col, int row_count, int col_count, int dx, int dy);

public:
    Grid(const BoxDef& b, const Geometry& g, int strip_lines=0) : bdef(b), geom(&g), strip_lines(strip_lines) {}

    // Callback for cell text. Returns NULL if no text required
    virtual const char* get_text_dos(const MiniCell&) = 0;

    ///////////// Run time coordinates info //////////////////////////

    // Returns coordinates of outer box
    Rect get_coord() const {return bounds;}

    // Return rectangle of cell outer part
    Rect get_cell_coord_ext(int row, int col) const {return cell(row, col).box;}

    // Return rectangle of cell internals
    Rect get_cell_coord_int(int row, int col) const {return shrink(cell(row, col).box, bdef.cell_box_def);}

    // Return reserved space Rect
    Rect get_reserved(bool top_part=true)
    {
        initial_geom_eval();
        Rect inner = shrink(bounds, bdef.outer_box_def);
        if (top_part) inner.height = bdef.reserve_top;
        else inner.width = bdef.reserve_left;
        return inner;
    }

    ////////////// Touch interface ///////////////////
    // All fields is -1 if no touch
    struct TouchCell {
        int row = -1;
        int col = -1;
        int id = -1;
    };
    TouchCell get_touch(int x, int y) const;

    ///////////// Runtime geomentry/contents manipulation ////////////////////////////////
    void swap_geometry(Geometry*);
    void set_cell_id(int row, int col, int id) {initial_geom_eval(); auto& c = cell(row, col); if (c.id != id) {c.id = id; c.updated = true;}}
    int get_cell_id(int row, int col) {initial_geom_eval(); return cell(row, col).id;}

    ////////////// Updates management /////////////////////////////////////////////////////
    // Mark cells to redraw.
    void invalidate(int row, int col, int row_count=1, int col_count=1)
    {
        initial_geom_eval();
        for(int r=0; r<row_count; ++r)
            for(int c=0; c<col_count; ++c)
                cell(r, c).updated = true;
    }

    bool need_update() const
    {
        for(const auto& c: cells) if (c.updated) return true;
        return false;
    }

    ///////// Draw members ////////////////////////

    // Draw subgrid (row/col by row_count/col_count cells) with shift dx/dy from its original position.
    // Keep track of dx/dy changes to clear possible tails of previous subgrid images
    // History cleared by 'update' call
    void draw_float(LCD&, int row, int col, int row_count, int col_count, int dx, int dy);

    // Prepare to draw Grid inside supplied coordinates and Draw them
    void set_coord(LCD& lcd, const Rect&);

    // Draw grid on screen
    void update(LCD&, bool with_spacers=false);
};


} // namespace GridManager
