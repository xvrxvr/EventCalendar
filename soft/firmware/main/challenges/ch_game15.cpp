#include "common.h"
#include "grid_mgr.h"
#include "activity.h"
#include "interactive.h"
#include "challenge_list.h"

#define Lcd() Activity::LCDAccess(NULL).access()

namespace Game15 {

using namespace GridManager;


Geometry game_geom( Rows() 
    << Row()(1)(2)(3)(4)
    << Row()(5)(6)(7)(8)
    << Row()(9)(10)(11)(12)
    << Row()(13)(14)(15)
);

BoxDef game_bdef = {
    .box_defs = {
        "5555155U0A630E03 D6D9,0000,9CD3,FFFF", //Box
        "2255103U0A630E03 FFFF,0000,9CD3,0000"  // Tile
    },
    .cell_width = 50,
    .cell_height = 30
};

struct Point {
    int row=-1, col=-1;
};

// Trim value by to bounds
struct CTrim {
    int min_value=0;
    int max_value=100000;

    int operator()(int v) const {return std::max(min_value, std::min(max_value, v));}

    void operator =(int v) {min_value = max_value = v;}
    void operator -=(int v) {min_value -= v;}
    void operator +=(int v) {max_value += v;}

    int dist() const {return max_value - min_value;}
};

// Deltas inside DragBlock - direction and range from gap to selected cell
struct DBDeltas {
    int d_row; // Direction from Gap to Row (-1/0/1)
    int d_col; // Direction from Gap to Col (-1/0/1)
    int total; // Disatnce from Gap to Cell (excluding Gap)
};

// Represenation of dragged block of cells
struct DragBlock {
    int row=-1, col=-1; // Row & Col of touch point (one point of drag, other - gap position)
    int x=-1, y=-1; // where touch was detected

    CTrim lim_x, lim_y; // Allowed coordinates for x & y during drag

    bool active() const {return row != -1;}
    void clear() {row=-1;}
};

class Game15 : public Grid {
    char txt_buf[5];
    DragBlock cur_drag;
    int move_count = 0;
    bool help_active = false;
    int last_x=0, last_y=0; // Where last touch/drag occure
    
    Point find(int idx) const
    {
        for(int r=0; r<4; ++r)
            for(int c=0; c<4; ++c)
                if (get_cell_id(r, c) == idx) 
                    return Point{r, c};
        return {};
    }

    virtual const char* get_text_dos(const MiniCell& cell) override
    {
        if (cell.id == -1) return NULL;
        sprintf(txt_buf," %d ", cell.id);
        return txt_buf;
    }

    bool is_can_move(int row, int col) const {const auto gap = find(-1); return ((row==gap.row) + (col==gap.col)) == 1;}

    DragBlock block_rc(int row, int col) const
    {
        assert(is_can_move(row, col));
        return DragBlock{
            .row = row,
            .col = col
        };
    }

    DragBlock block_xy(int x, int y) const;

    DBDeltas get_deltas(const DragBlock&) const;

    void apply(const DragBlock&);

    void random_init();

    // Draw drag block on screen
    void do_block_drag(int x=-1, int y=-1);

    // Test if drag came to new place (run on touch release)
    bool is_new_place(int x, int y) const
    {
        int dx = abs(cur_drag.lim_x(x)-cur_drag.x);
        int dy = abs(cur_drag.lim_y(y)-cur_drag.y);
        return dx > cur_drag.lim_x.dist()/2 || dy > cur_drag.lim_y.dist()/2;
    }

    bool is_game_finished()
    {
        int idx=1;
        for(int r=0; r<4; ++r)
            for(int c=0; c<4; ++c, ++idx)
                if (idx < 16 && get_cell_id(r, c) != idx) 
                    return false;
        return true;
    }

    // Return true if processed
    bool on_touch(const Action& act);

public:
    Game15() : Grid(game_bdef, game_geom) {}

    int run();
};

// Return true if processed
bool Game15::on_touch(const Action& act)
{
    switch(act.type)
    {
        case AT_TouchDown:         
            cur_drag = block_xy(act.touch.x, act.touch.y);
            last_x = act.touch.x;
            last_y = act.touch.y;
            return cur_drag.active();
        case AT_TouchTrack:
            if (!cur_drag.active()) return false;
            do_block_drag(act.touch.x, act.touch.y);
            last_x = act.touch.x;
            last_y = act.touch.y;
            return true;
        case AT_TouchUp:
            if (!cur_drag.active()) return false;
            if (is_new_place(last_x, last_y)) 
            {
                apply(cur_drag); 
                ++move_count;
                if (!help_active && move_count >= 10)
                {
                    help_active = true;
                    Interactive::draw_help_icon();
                }
            }
            cur_drag.clear();
            update(Lcd(), true);
            return true;
        default: return false;
    }
}

inline int sign(int v) {return v > 0 ? 1 : v < 0 ? -1 : 0;}

void Game15::apply(const DragBlock& blk)
{
    if (blk.row == -1) return;
    DBDeltas d = get_deltas(blk);
    Point gap = find(-1); 
    while(d.total--)
    {
        set_cell_id(gap.row, gap.col, get_cell_id(gap.row+d.d_row, gap.col+d.d_col));
        gap.row += d.d_row;
        gap.col += d.d_col;
    }
    set_cell_id(gap.row, gap.col, -1);
}

DBDeltas Game15::get_deltas(const DragBlock& blk) const
{
    const Point gap = find(-1); 
    auto d_row = blk.row - gap.row;
    auto d_col = blk.col - gap.col;
    return DBDeltas{
        .d_row = sign(d_row),
        .d_col = sign(d_col),
        .total = abs(d_row) + abs(d_col)
    };
}

DragBlock Game15::block_xy(int x, int y) const
{
    auto touch = get_touch(x,y);
    if (touch.id == -1 || !is_can_move(touch.row, touch.col)) return {};
    auto result = block_rc(touch.row, touch.col);
    result.x = x;
    result.y = y;
    result.lim_x = x;
    result.lim_y = y;

    const Point gap = find(-1); 
    auto cell_rect = get_cell_coord_ext(gap.row, gap.col);
    if (result.row == gap.row) // Horisontal
    {
        if (result.col < gap.col) result.lim_x += cell_rect.width + box_defs[1].marging_h; // Can move right
        else result.lim_x -= cell_rect.width + box_defs[1].marging_h; // Can move left
    }
    else // Vertical
    {
        if (result.row < gap.row) result.lim_y += cell_rect.height + box_defs[1].marging_v; // Can move down
        else result.lim_y -= cell_rect.height + box_defs[1].marging_v; // Can move up
    }
    return result;
}

// Draw drag block on screen
void Game15::do_block_drag(int x, int y)
{
    int dx = cur_drag.lim_x(x) - cur_drag.x;
    int dy = cur_drag.lim_y(y) - cur_drag.y;

    DBDeltas d = get_deltas(cur_drag);
    const auto gap = find(-1);

    int total_cols = std::max(1, d.total * abs(d.d_col));
    int total_rows = std::max(1, d.total * abs(d.d_row));
    int row = std::min(gap.row+d.d_row, cur_drag.row);
    int col = std::min(gap.col+d.d_col, cur_drag.col);

    draw_float(Lcd(), row, col, total_rows, total_cols, dx, dy);
}

inline void rnd_move(int& val)
{
    int new_val = esp_random() % 3;
    if (new_val >= val) ++new_val;
    val = new_val;
}

void Game15::random_init()
{
    get_coord();
    for(int i=0; i<100; ++i)
    {
        Point gap = find(-1);
        if (esp_random() & 1) rnd_move(gap.row); else rnd_move(gap.col);
        apply(block_rc(gap.row, gap.col));
    }
}

int Game15::run()
{
    random_init();
    set_coord(Lcd(), GridManager::Rect{0, 0, RES_X, RES_Y});

    Activity act(AT_TouchDown|AT_TouchTrack|AT_TouchUp|AT_WatchDog);
    act.setup_watchdog(s2ticks(SC_TurnoffDelay));

    for(;;)
    {
        Action a = act.get_action();
        on_touch(a);
        if (help_active && a.type == AT_TouchDown && Interactive::test_help_icon(a))
        {
            act.add_actions(AT_Fingerprint2);
        }
        switch(a.type)
        {
            case AT_TouchUp: if (is_game_finished()) return CR_Ok; else break;
            case AT_WatchDog: return CR_Timeout;
            case AT_Fingerprint: if (Interactive::check_open_door_fingerprint(a)) return CR_Ok; //!! Make auto solver
            default: break;
        }
    }
}

int run_challenge()
{
    return Game15().run();
}


} // namespace Game15
