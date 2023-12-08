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

    bool test_answer(const char*);

    // Init all Quest subsystem. Select Quest number to ask.
    // Returns quest number or CR_Error if can't select
    int init();

    // Load Challenge, Draw question, wait for touch
    // Return CR_* on error, 0 if Ok
    int ask_challenge(int index);

    // Run keyboard input method
    // Return CR_* or 0 if user click Help icon
    int get_ans_keyboard();

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

int Riddle::ask_challenge(int index)
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
    parser.parse_text(chf.get_body());

    bg_images.draw(Lcd());
    parser.draw_one_box_centered(Lcd());

    Activity act(AT_TouchDown|AT_WatchDog);
    act.setup_watchdog(s2ticks(SC_TurnoffDelay));

    return act.get_action().type == AT_WatchDog ? CR_Timeout : 0;
}

void Riddle::setup_kb_type()
{
    kb_type = td.keyb_type();
    if (kb_type & TextBoxDraw::GO_KbSelector) return;
    for(uint8_t s: chf.get_ans())
    {
        if (isdigit(s) || s == '.' || s == ',') kb_type |= TextBoxDraw::GO_KbNumbers; else
        if (s == ' ' || s == '\n') ; else
        if (s & 0x80) kb_type |= TextBoxDraw::GO_KbRussian;
        else kb_type |= TextBoxDraw::GO_KbEnglish;
        if ((kb_type & TextBoxDraw::GO_KbMask) == (TextBoxDraw::GO_KbMask ^ TextBoxDraw::GO_KbSelector)) break;
    }
}

int Riddle::get_ans_keyboard()
{
    static const GridManager::KeybBoxDef bdef{
        .box_def{
            .box_defs = {KB_PALLETE},
            .reserve_top = 20
        }
    };

    GridManager::Keyboard kb(bdef, kb_type);
    bg_images.draw(Lcd());
    kb.set_coord(Lcd(), GridManager::Rect{0, 0, RES_X, RES_Y});

    return kb.default_kb_process([&, this]() {return test_answer(kb.kb_get_string());});
}

bool Riddle::test_answer(const char* answer)
{
    int length;
    auto score = DVPlus::compare(chf.get_valid_ans(), std::string_view(answer), length);
    int limit = td.fuzzy_dist;
    if (td.fuzzy_is_percent()) limit = length * limit / 100;
    return score <= limit;
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

    // Init all Quest subsystem. Select Quest number to ask.
    // Returns quest number or CR_Error if can't select
    int ch_index = init();
    if (ch_index < 0) return ch_index; // This is error

    do {
        // Load Challenge, Draw question, wait for touch
        // Return CR_* on error, 0 if Ok
        err = ask_challenge(ch_index);
        if (err) break;

        if (!(kb_type & TextBoxDraw::GO_KbSelector))
        {
            // Run keyboard input method
            // Return CR_* or 0 if user click Help icon
            err = get_ans_keyboard();
            if (err) break;
        }
        // Run selection. Return CR_*
        err = get_selection();
    } while(false);

    if (err == CR_Ok) finalize(ch_index);

    return err;
}

int run_challenge()
{
    std::unique_ptr<Riddle> riddle(new Riddle); // Create riddle on heap, because it quite big and can not fit in stack
    return riddle->run();
}

} // namespace Quest