#include "common.h"
#include "hadrware.h"
#include "setup_data.h"
#include "challenge_mgr.h"
#include "challenge_list.h"
#include "text_draw.h"
#include "activity.h"
#include "bg_image.h"
#include "keyboard.h"
#include "icons.h"
#include "fazzy_match.h"
#include "multi_select.h"

namespace Riddle {

static const char TAG[] = "riddle";

class Riddle {
    uint8_t used_quests[32];
    ChFile chf;
    TextBoxDraw::TextGlobalDefinition td;
    int kb_type = 0;

    bool test_ch(uint8_t index) {return (used_quests[index>>3] & bit(index&7)) != 0;}
    void set_ch(uint8_t index) 
    {
        int idx = index>>3;
        used_quests[idx] |= bit(index&7);
        EEPROM::write((ES_UsedQ + logged_in_user)*EEPROM::page_size + idx, used_quests + idx, 1);
    }

    void setup_kb_type();

    bool test_answer(GridManager::Keyboard&);

    // Init all Quest subsystem. Select Quest number to ask.
    // Returns quest number or CR_Error if can't select
    int init();

    // Ask riddle and enter answer
    // Return CR_* or 0 if user click Help icon
    int ask_and_query(int index);

    // Run selection. Return CR_*
    int get_selection();

    // Finalize successful Riddle
    void finalize(int ch_index) 
    {
        set_ch(ch_index);
    }

public:

    // Full run
    int run();
};

int Riddle::init()
{
    if (logged_in_user < 0) 
    {
        ESP_LOGE(TAG, "No logged-in user");
        return -1;
//        logged_in_user = 0;
    }
    EEPROM::read_pg(ES_UsedQ + logged_in_user, used_quests, sizeof(used_quests));
    challenge_mgr().shuffle_challenges();
    for(const auto& ch: challenge_mgr().all_data())
    {
        if (ch.second == logged_in_user || test_ch(ch.first)) continue;
        return ch.first;
    }
    ESP_LOGI(TAG, "No more Riddles for this User");
    return CR_Error;
}

#define Lcd() Activity::LCDAccess(NULL).access()

int Riddle::ask_and_query(int index)
{
    if (!challenge_mgr().read_ch_file(chf, index))
    {
        ESP_LOGE(TAG, "Error reading Riddle file #%d", index);
        return CR_Error;
    }
    auto hdr = chf.get_header();
    td.setup(hdr.data());
    setup_kb_type();

    TextBoxDraw::TextsParser parser(td);
    GridManager::Keyboard kb(kb_type);
    parser.parse_text(chf.get_body());

    bool sequential = true;

    auto box_rect = parser.min_box_size();
    auto kb_size = kb.get_coord();

    int kb_y = 0, kb_height = RES_Y, box_height = RES_Y;

    if (!(kb_type & TextBoxDraw::GO_KbSelector) && box_rect.second + kb_size.height + td.marging_v*3 <= RES_Y)
    {
        // Run in one screen
        sequential = false;
        auto extra = (RES_Y - td.marging_v - box_rect.second - kb_size.height) / 2;
        kb_height = kb_size.height + extra;
        box_height = box_rect.second + extra;
        kb_y = box_height + td.marging_v;
    }

    bg_images.draw(Lcd());
    parser.draw_one_box_centered(Lcd(), 0, 0, RES_X, box_height);
    if (sequential)
    {
        Activity act(AT_TouchDown|AT_WatchDog);
        act.setup_watchdog(s2ticks(SC_TurnoffDelay));
        if (act.get_action().type == AT_WatchDog) return CR_Timeout;

        if (kb_type & TextBoxDraw::GO_KbSelector) return 0;
        bg_images.draw(Lcd());
    }
    kb.set_coord(Lcd(), GridManager::Rect{0, kb_y, RES_X, kb_height});
    return kb.default_kb_process([&, this]() {return test_answer(kb);});
}

void Riddle::setup_kb_type()
{
    kb_type = td.keyb_type();
    if (kb_type) return;
    for(uint8_t s: chf.get_ans())
    {
        if (isdigit(s) || s == '.' || s == ',') kb_type |= TextBoxDraw::GO_KbNumbers; else
        if (s == ' ' || s == '\n') ; else
        if (s & 0x80) kb_type |= TextBoxDraw::GO_KbRussian;
        else kb_type |= TextBoxDraw::GO_KbEnglish;
        if ((kb_type & TextBoxDraw::GO_KbMask) == (TextBoxDraw::GO_KbMask ^ TextBoxDraw::GO_KbSelector)) break;
    }
}

bool Riddle::test_answer(GridManager::Keyboard& kb)
{
    int length;
    const char* answer = kb.kb_get_string();
    auto org = chf.get_valid_ans();
    auto score = DVPlus::compare(org, std::string_view(answer), length);
    int limit = td.fuzzy_dist;
    if (td.fuzzy_is_percent()) limit = length * limit / 100;
    if (score > limit) return false;
    if (answer != org) kb.message_dos(Lcd(), org);
    return true;
}

// Run selection. Return CR_*
int Riddle::get_selection()
{
    TextBoxDraw::TextsParser parser(td);
    parser.parse_text(chf.get_ans());

    try {
        return multi_select_const(chf.get_ans(), td) ? CR_Ok : CR_Timeout;
    }
    catch(TextBoxDraw::TextModuleErrors&) {
        ESP_LOGE(TAG, "Answers not fitted in screen");
        return CR_Error;
    }
}

int Riddle::run()
{
    int err;

    int ch_index = init();
    if (ch_index < 0) return ch_index; // This is error

    err = ask_and_query(ch_index);
    if (!err) err = get_selection();

    if (err == CR_Ok) finalize(ch_index);

    return err;
}

int run_challenge()
{
    std::unique_ptr<Riddle> riddle(new Riddle); // Create riddle on heap, because it quite big and can not fit in stack
    return riddle->run();
}

} // namespace Quest