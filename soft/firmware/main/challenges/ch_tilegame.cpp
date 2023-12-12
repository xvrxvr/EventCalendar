#include "common.h"
#include "grid_mgr.h"
#include "activity.h"
#include "interactive.h"
#include "challenge_list.h"
#include "bg_image.h"
#include "animation.h"

namespace TileGame {

#define Lcd() Activity::LCDAccess(NULL).access()

using namespace GridManager; 

Geometry game_geom( Rows() 
    << Row()(0)(0)(0)(0)
    << Row()(0)(0)(0)(0)
    << Row()(0)(0)(0)(0)
    << Row()(0)(0)(0)(0)    
);

BoxDef game_bdef = {
    .box_defs = {
        "5555155U0A630E03 D6D9,0000,9CD3,FFFF", //Box
        "2255103U0A630E03 FFFF,0000,9CD3,0000", //Tile - On
        "2255103U0A630E03 8C51,0000,9CD3,0000", //Tile - Off
        "2255103U0A630E03 FFFF,0000,9CD3,0000", //Dulicate - for animation (Tile - On)
        "2255103U0A630E03 8C51,0000,9CD3,0000"  //Dulicate - for animation (Tile - Off)
    },
    .cell_width = 50,
    .cell_height = 30
};


class TileGame : public Grid {
    bool help_active = false;
    int move_count = 0;

    virtual const char* get_text_dos(const MiniCell&) override {return NULL;}

    void invert(int row, int col, bool update=true);
    bool solved() const;
    void init();
    void flash_green();

public:
    TileGame() : Grid(game_bdef, game_geom) {}

    int run();
};

void TileGame::flash_green()
{
    Animation a1{.anim={
        .type = AT_Pulse,
        .color_from=box_defs[1].bg_color,
        .color_to=0x27E8,
        .length=5
    }, .stage=0};
    while(a1.is_active())
    {
        box_defs[1].bg_color = a1.animate(true);
        vTaskDelay(SC_FGEditAnimSpeed);
        invalidate(0, 0, UI_Box, 4, 4);
        this->update(Lcd());
    }
}


void TileGame::invert(int row, int col, bool update)
{
    // Box 0->2, 1->3 or 0->1, 1->0 if no update
    auto upd = [&](int row, int col) {
        int box_id = get_cell_box(row, col);
        if (update) box_id += 2; else box_id = 1-box_id;
        set_cell_box(row, col, box_id);
    };
    for(int r=0; r<4; ++r) upd(r, col);
    for(int c=0; c<4; ++c) if (c != col) upd(row, c);

    if (update) 
    {
        Animation a1{.anim={
            .type = AT_One,
            .color_from=box_defs[1].bg_color,
            .color_to=box_defs[2].bg_color,
            .length=5
        }, .stage=0};
        Animation a2(a1);
        std::swap(a2.anim.color_from, a2.anim.color_to);

        while(a1.is_active())
        {
            box_defs[3].bg_color = a1.animate(true);
            box_defs[4].bg_color = a2.animate(true);
            vTaskDelay(SC_FGEditAnimSpeed);
            this->update(Lcd());
            invalidate(0, col, UI_Box, 4, 1);
            invalidate(row, 0, UI_Box, 1, 4);
        }

        // Box 2->1, 3->0
        for(int r=0; r<4; ++r) {set_cell_box(r, col, 3-get_cell_box(r, col));}
        for(int c=0; c<4; ++c) if (c != col) {set_cell_box(row, c, 3-get_cell_box(row, c));}
        this->update(Lcd());
    }
}

bool TileGame::solved() const
{
    for(int r=0; r<4; ++r)
        for(int c=0; c<4; ++c)
            if (get_cell_box(r,c) != 0) return false;
    return true;
}

void TileGame::init()
{
    for(int i=0;i<100;++i) invert(esp_random() & 3, esp_random() & 3, false);
}

int TileGame::run()
{
    get_coord();
    init();
    set_coord(Lcd(), GridManager::Rect{0, 0, RES_X, RES_Y});

    Activity act(AT_TouchDown|AT_WatchDog);
    act.setup_watchdog(s2ticks(SC_TurnoffDelay));

    for(;;)
    {
        Action a = act.get_action();
        if (help_active && a.type == AT_TouchDown && Interactive::test_help_icon(a))
        {
            act.add_actions(AT_Fingerprint2);
            continue;
        }
        switch(a.type)
        {
            case AT_TouchDown: 
            {
                auto touch = get_touch(a.touch.x, a.touch.y);
                if (touch.id == -1) break;
                invert(touch.row, touch.col);
                if (solved()) {flash_green(); return CR_Ok;}
                ++move_count;
                if (!help_active && move_count >= 10)
                {
                    help_active = true;
                    Interactive::draw_help_icon();
                }
                break;
            }
            case AT_WatchDog: return CR_Timeout;
            case AT_Fingerprint: if (Interactive::check_open_door_fingerprint(a)) return CR_Ok; //!! Make auto solver
            default: break;
        }
    }
}

int run_challenge()
{
    bg_images.draw(Lcd());
    std::unique_ptr<TileGame> g(new TileGame);
    return g->run();
}


} // namespace TileGhame