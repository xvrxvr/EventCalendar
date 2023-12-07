#include "common.h"
#include "equest.h"
#include "setup_data.h"
#include "keyboard.h"
#include "activity.h"
#include "bg_image.h"
#include "challenge_list.h"
#include "icons.h"
#include "multi_select.h"

namespace EQuest {

struct LevelDef {
    std::initializer_list<int> v_range_list;
    int min_opc;
    int max_opc;
    int min_age;
    int max_age;
};

static LevelDef levels[] = {
    {{1}, 1, 1, 0, 10},
    {{2,1}, 1, 1, 0, 10},
    {{1,2}, 1, 2, 6, 15},
    {{1,2}, 1, 3, 8, 30},
    {{0,2,1}, 1, 3, 20, 99},
    {{0,2,1}, 2, 5, 20, 99},
    {{0,1,2}, 3, 5, 30, 99}
};

// Get index into 'levels' for currently logged in user
static int get_level()
{
    int idx[std::size(levels)];
    int index=0, level_idx=0;
    for(const auto& L: levels)
    {
        if (L.min_age <= current_user.age && current_user.age <= L.max_age) idx[index++] = level_idx;
        ++level_idx;
    }
    if (!index) return 0;
    return idx[esp_random() % index];
}

static int get_random_equest(Prn& result, int allowed_width)
{
    int level = get_level();
    int subtries = 0;
    while(level>=0)
    {
        const LevelDef& L = levels[level];
        const int op_count = esp_random() % (L.max_opc-L.min_opc+1) + L.min_opc;
        auto tree = Generator(L.v_range_list).create_tree(op_count);
        if (!tree) {--level; subtries = 0; continue;}
        result.strcpy("");
        tree->to_string(result);
        if (result.length() > allowed_width) 
        {
            if (++subtries > 100) {subtries = 0; --level;}
            continue;
        }
        return tree->get_val();
    }
    return -1;
}

// <red>, <green>
static uint16_t colors[] = {0xF904, 0x27E8};

// Result - ChResults or value to pass to MultiSelect dialog
static int run_challenge_p1()
{
    GridManager::KeybBoxDef bdef{
        .box_def{
            .box_defs = {
                "5555155U0A630E03 D6D9,0000,9CD3,FFFF", // Box
                "2211003U0A630E03 1BE3,0000,9CD3,FFFF",// Keyboard
                "5555155U0A630E03 D6D9,0000,9CD3,0000", // Use it for text draw
            },
            .reserve_top = 40, // -1 for reserve all availabe space
            .reserve_left = -1, // -1 for reserve all availabe space
        },
        .kb_options = GridManager::KBO_Left
    };
    Activity act(AT_TouchDown|AT_WatchDog); // Add timeout! (Make separate microtick support)
    act.setup_watchdog_ticks(50);
#define lcd Activity::LCDAccess(&act).access()

    bg_images.draw(lcd);

    GridManager::Keyboard kb(bdef, GridManager::keyb_digits, GridManager::keyb_digits);
    kb.set_coord(lcd, GridManager::Rect{0, 0, 400, 240});

    auto rect = kb.get_reserved();
    int total_syms = rect.width/8;
    bool help_active = false;
    int error_count = 0;    
    Prn prn("Сколько будет?");

    utf8_to_dos(prn.c_str());
    kb.set_lcd_color(lcd, 2);
    lcd.text(prn.c_str(), rect.x + (rect.width - strlen(prn.c_str())*8)/2, rect.y);

    int value = get_random_equest(prn, total_syms);
    if (value == -1) return CR_Error;
    lcd.text(prn.c_str(), rect.x + (rect.width - prn.length()*8)/2, rect.y+16);

    kb.kb_activate(lcd);
    for(;;)
    {
        auto a = act.get_action();
        if (a.type == AT_TouchDown)
        {
            int id = kb.get_touch(a.touch.x, a.touch.y).id;
            if (id == -1)
            {
                if (help_active && a.touch.y < 32 && a.touch.x > RES_X-32) return value;
                continue;
            } 
            if (kb.kb_process(lcd, id))
            {
                if (atoi(kb.kb_get_string()) == value)
                {
                    kb.message_utf8(lcd, "\3Правильно", colors);
                    return CR_Ok;
                }
                else
                {
                    kb.message_utf8(lcd, "\2Не правильно\3 Ещё раз ...", colors);
                    ++error_count;
                    if (error_count == 3)
                    {
                        help_active = true;
                        lcd.icon32x32(RES_X-32, 0, help_icon, 0x27E8);
                    }
                }
            }
        } else
        if (a.type == AT_WatchDog) kb.kb_animate(lcd);
    }
}

static ChResults run_multi_selection(int value)
{
    Prn buf;
    int delta = std::max(5, value/10);
    auto rnd = [=]() {return (esp_random() % delta) + 1;};

    return multi_select_vary([&]() -> std::string_view {
        buf.printf("%d\n", value);
        int r1 = rnd();
        buf.cat_printf("%d\n", value + r1);
        if (r1 < value) buf.cat_printf("%d\n", value - r1);
        else {r1 += rnd(); buf.cat_printf("%d\n", value + r1);}
        r1 += rnd();
        if (r1 < value && (esp_random() & 1)) r1 = -r1;
        buf.cat_printf("%d\n", value + r1);
        return std::string_view(buf.c_str(), buf.length());
    }, TextBoxDraw::default_text_global_definition) ? CR_Ok : CR_Timeout;
}

// Result - ChResults
int run_challenge()
{
    int result = run_challenge_p1();
    if (result < 0) return result;
    return run_multi_selection(result);
}

} // namespace EQuest
