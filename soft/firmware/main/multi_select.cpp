#include "common.h"
#include "multi_select.h"
#include "activity.h"
#include "bg_image.h"
#include "interactive.h"

#include <random>

enum ResultType {
    RT_Valid,
    RT_Invalid,
    RT_Timeout
};

static ResultType select_one(TextBoxDraw::TextsParser& parser, std::vector<TextBoxDraw::CellDef>& cdef, int x, int y, int width, int height)
{
    {
        Activity::LCDAccess acc(NULL);
        bg_images.draw(acc.access());
        parser.draw_selection_of_boxes(acc.access(), cdef.data(), std::min<size_t>(4, cdef.size()), x, y, width, height);
    }
    Activity act(AT_TouchDown|AT_WatchDog);
    act.setup_watchdog(SC_TurnoffDelay);

    for(;;)
    {
        Action a = act.get_action();
        if (a.type == AT_WatchDog) return RT_Timeout;
        for(const auto& c: cdef)
        {
            if (a.touch.x >=c.x && a.touch.x < c.x+c.width && a.touch.y >= c.y && a.touch.y < c.y+c.height) return c.index == 0 ? RT_Valid : RT_Invalid;
        }
    }
}

static ResultType run_round(TextBoxDraw::TextsParser& parser, bool multi_run, int x, int y, int width, int height)
{
    int sz = parser.total_lines();
    std::vector<TextBoxDraw::CellDef> cdef;
    std::minstd_rand rnd(esp_random());

    cdef.reserve(sz);
    for(int i=0; i<sz; ++i) cdef.push_back({i});

    for(;;)
    {
        std::shuffle(cdef.begin(), cdef.end(), rnd);
        if (sz > 4)
        {
            int index;
            for(index=0; index<sz; ++index) if (cdef[index].index==0) break;
            if (index >= 4) std::swap(cdef[esp_random() & 3], cdef[index]);
        }
        auto result = select_one(parser, cdef, x, y, width, height);
        switch(result)
        {
            case RT_Invalid:
                Interactive::lcd_message("\\#\\cF904\\Неверно!\n\\#Ещё раз ...");
                vTaskDelay(s2ticks(SC_MultiSelectErr));
                if (!multi_run) return RT_Invalid;
                break;
            case RT_Valid:
                Interactive::lcd_message("Правильно");
                vTaskDelay(s2ticks(SC_MultiSelectErr));
                return RT_Valid;
            default: return result;
        }
    }
}

// Returns 'false' on timeout
bool multi_select_const(const std::string_view& data, const TextBoxDraw::TextGlobalDefinition &td, int x, int y, int width, int height)
{
    TextBoxDraw::TextsParser parser(td);
    parser.parse_text(data);
    return run_round(parser, true, x, y, width, height) == RT_Valid;
}

// Returns 'false' on timeout
bool multi_select_vary(std::function<std::string_view()> new_data, const TextBoxDraw::TextGlobalDefinition &td, int x, int y, int width, int height)
{
    for(;;)
    {
        TextBoxDraw::TextsParser parser(td);
        parser.parse_text(new_data());
        auto result = run_round(parser, false, x, y, width, height);
        if (result != RT_Invalid) return result == RT_Valid;
    }
}
