#include "common.h"
#include "setup_data.h"
#include "wifi_module.h"
#include "prnbuf.h"
#include "text_draw.h"
#include "bg_image.h"
#include "web_vars.h"
#include "web_gadgets.h"
#include "animation.h"
#include "icons.h"
#include "interactive.h"
#include "challenge_list.h"

extern uint32_t my_ip;

static const char TAG[] = "interactive";
namespace Interactive {

// Preallocate new user, returns index. Returns -1 if no more users
static int fge_allocate_user()
{
    if (!(current_user.options & (UO_CanAddRemoveUser|UO_CanAddRemoveAdmin))) return -1;
    uint32_t sc = ~get_eeprom_users();
    if (sc == 0) return -1;
    // __builtin_ctz - Returns the number of trailing 0-bits in x, starting at the least significant bit position.
    return __builtin_ctz(sc);
}

inline void do_fg_del(int index, uint8_t count=1)
{
    if (current_user.options & UO_CanEditFG) ::do_fg_del(index, count);
    // else - send error via AJAX
}

// Activate (create) new user. 'name' in DOS
static void fge_activate_user(int user_index, const char* name, int age)
{
    char b[33];

    if (!(current_user.options & (UO_CanAddRemoveUser|UO_CanAddRemoveAdmin))) return;
    UserSetup usr = current_user;

    strncpy(b, name, 32);
    b[32] = 0;
    if (!(current_user.options & UO_CanAddRemoveAdmin)) usr.options = 0;
    usr.age = age;
    usr.save(user_index, (uint8_t*)b);
    ESP_LOGI(TAG, "New user %d: %s", user_index, name);
}

// Draw message (in UTF8) to LCD in centered Box
// If 'msg' is NULL draws last box + mesage on already locked LCD instance
void lcd_message(const char* msg, ...)
{
    static Prn buf;
    static TickType_t last_draw_time;

    TextBoxDraw::TextsParser pars(TextBoxDraw::default_text_global_definition);
    va_list l;

    if (msg)
    {
        va_start(l, msg);
        buf.vprintf(msg, l);
        va_end(l);
        if (bg_images.is_durty()) 
        {
            auto delta = xTaskGetTickCount() - last_draw_time;
            if (delta < ms2ticks(SC_MinMsgTime)) vTaskDelay(ms2ticks(SC_MinMsgTime) - delta);
            bg_images.draw(Activity::LCDAccess(NULL).access());
        }
        utf8_to_dos(buf.c_str());
        last_draw_time = xTaskGetTickCount();
    }

    pars.parse_text(buf.c_str());

    if (msg) pars.draw_one_box_centered(Activity::LCDAccess(NULL).access());
    else pars.draw_one_box_centered(lcd);

    bg_images.set_durty();
}

void msg_valid(bool is_valid)
{
    lcd_message(is_valid ? "\\2Правильно" : "\\#\\2\\cF904\\Неверно!\n\\#\\2Ещё раз ...");
    vTaskDelay(s2ticks(SC_MultiSelectErr));
}

class MsgActivity : public Activity {
public:
    using Activity::Activity;

    virtual void update_scene(LCD &lcd) override
    {
        bg_images.draw(lcd);
        lcd_message(NULL);
    }
};

// Turn off LCD backlight
inline void lcd_teardown() {fade_out();}
// Turn on LCD background
inline void lcd_turnon() {fade_in();}

// Restart WEB page
inline void restart_web_page(const char* url) {web_send_cmd("{'cmd':'goto','href':'%s'}", url);}

// Setup WEB root as Mesage with 'message' and 'title'
inline void set_web_message(const char* title, const char* message) 
{
    web_options.set_title_and_message(title, message);
    set_web_root("/web/message.html");
}

static bool game();
static void no_game();
static void challenge();

static void fg_edit(int user_index);
static void fg_view();

static void web_event_process(const Action& act);

void entry()
{
    bg_images.peek_next_bg_image();
    bg_images.draw(Activity::LCDAccess(NULL).access());

    set_web_root("/web/message.html");        

    switch(working_state.state)
    {
        case WS_Pending:
            if (working_state.last_round_time <= utc2ts(time(NULL)))
            {
                start_game();
                if (game()) challenge();
                return;
            }
            lcd_message("Приходите в %s", ts_to_string(working_state.last_round_time)); 
            no_game(); 
            return;
        case WS_Active: if (game()) challenge(); return;
        //case WS_NotActive: 
        default: break;
    }
    if (override_switch_active() || !get_eeprom_users())
    {
        login_superuser();
        set_web_root("/web/admin.html");        
        do {
            lcd_message("Ждите настройки");
            web_event_process(MsgActivity(AT_WEBEvent).get_action());
        } while(working_state.state == WS_NotActive && (override_switch_active() || !get_eeprom_users()));
        return;
    }
    set_web_message("Waiting for login", "Пожалуста, войдите в систему с помощью отпечатка пальца");
    lcd_message("Раздача подарков ещё не начата, ждите");
    no_game();
}

static Action wait(uint32_t action)
{
    return MsgActivity(AF_Override | action).get_action();
}

static void passivate()
{
    lcd_teardown(); 
    wait(AT_Fingerprint0|AT_TouchDown); 
    lcd_turnon();
    logged_in_user = -1;
}

static void web_event_process(const Action& act)
{
    switch(act.web.event)
    {
        case WE_FGDel:  ESP_LOGI(TAG, "web_event_process: FG Del (%d)", act.web.p1); do_fg_del(act.web.p1); return;
        case WE_FGEdit: ESP_LOGI(TAG, "web_event_process: FG Edit (%d)", act.web.p1); fg_edit(act.web.p1); break;
        case WE_FGView: ESP_LOGI(TAG, "web_event_process: FG View"); fg_view(); break;
        default: return;
    }
    bg_images.draw(Activity::LCDAccess(NULL).access());
}


#define AP_CRED "\\2\\#AP\nSSID: '" MASTER_WIFI_SSID "', Passwd: '" MASTER_WIFI_PASSWD "'\nhttp://192.168.4.1:8080\nhttp://event-calendar[.local]:8080"
static void draw_login_credentials()
{
    lcd_message(my_ip ? "\\2\\#Station\nhttp://%s\nhttp://event-calendar[.local]\n" AP_CRED : AP_CRED, inet_ntoa(my_ip));
}

static Action no_game_internal()
{
    bool enable_web_events = logged_in_user != -1 && current_user.options;
    MsgActivity act(AT_WatchDog|AT_Fingerprint0|(working_state.state == WS_Pending ? AT_Alarm : 0)|AT_WEBEvent|AT_TouchDown);
    if (working_state.state == WS_Pending) act.setup_alarm_action(ts2utc(working_state.last_round_time));
    act.setup_watchdog(SC_TurnoffDelay);

    for(;;)
    {
        Action a =  act.get_action();
        switch(a.type)
        {
            case AT_WatchDog: passivate(); return {};
            case AT_TouchDown: draw_login_credentials(); break;
            case AT_Fingerprint:
            {
                if (a.fp_index == -1) continue;
                login_user(a.fp_index>>2);
                if (!current_user.options) {enable_web_events = false; continue;}
                lcd_message("Настройка системы Админом");
                set_web_root("/web/admin.html");        
                restart_web_page("/");
                enable_web_events = true;
                break;
            }
            case AT_Alarm: start_game(); return {};
            case AT_WEBEvent:
                if (enable_web_events)
                {
                    switch(a.web.event)
                    {
                        case WE_Logout: case WE_GameStart: return {};
                        case WE_FGDel: case WE_FGEdit: case WE_FGView: return a;
                        default: break;
                    }
                }
                break;
            default: break;
        }        
    }
}

static void no_game()
{
    for(;;)
    {
        auto act = no_game_internal();
        if (act.type == AT_WEBEvent) web_event_process(act);
        else return;
    }
}

const char* test_user_login(const UserSetup& current_user, int logged_in_user)
{
    if ((current_user.status & (US_Enabled|US_Paricipated)) != (US_Enabled|US_Paricipated))
    {
        return "Вы не принимаете участия в раздаче подарков";
    }
    if (current_user.status & US_Done)
    {
        return "Вам уже вручили все подарки";
    }
    if (!(working_state.enabled_users & bit(logged_in_user)))
    {
        return "Вы уже получили свой подарок\nПриходите %s за следующим";
    }
    if (working_state.get_loaded_gift(logged_in_user) == -1)
    {
        return "Для вас не загрузили подарка\nСрочно поэовите Админа";
    }
    return NULL;
}

static bool game()
{
    MsgActivity act(AT_WatchDog|AT_Fingerprint|AT_WEBEvent|AT_TouchDown);
    act.setup_watchdog(SC_TurnoffDelay);
    bool test_user = (logged_in_user >= 0);

    for(;;)
    {
        const auto round = global_setup.round_time*60;
        uint32_t now = utc2ts(time(NULL));
        if (working_state.last_round_time + round <= now)
        {
            working_state.enabled_users = -1;
            working_state.last_round_time =  now - (now - working_state.last_round_time) % round;
            working_state.sync();
        }
        if (test_user)
        {
            test_user = false;
            bg_images.draw(Activity::LCDAccess(NULL).access());
            if (current_user.options) set_web_root("web/admin.html"); 
            else set_web_message("Access denied", "У вас недостаточно прав для входа сюда");
            const char* msg = test_user_login(current_user,logged_in_user);
            if (msg)
            {
                lcd_message(msg, ts_to_string(working_state.last_round_time + round));
                continue;
            }
            lcd_message("Вы готовы получить подарок?\nНо сначала загадка...");
            MsgActivity act(AT_WatchDog|AT_Fingerprint2|AT_WEBEvent|AT_TouchDown|AF_Override);
            act.setup_watchdog(SC_TurnoffDelay);
            Action a = act.get_action();
            switch(a.type)
            {
                case AT_WatchDog: passivate(); return false;
                case AT_Fingerprint: 
                    if (a.fp_index == -1 || logged_in_user == (a.fp_index>>2)) break;
                    login_user(a.fp_index>>2);
                    test_user = true;
                    continue;
                case AT_WEBEvent:
                    if (a.web.event == WE_GameEnd) return false;
                    // web_event_process(a); - No any WEB editors in Game
                    break;
                default: break;
            }
            return true;
        }
        Action a = act.get_action();
        switch(a.type)
        {
            case AT_WatchDog: passivate(); return false;
            case AT_Fingerprint: 
                if (a.fp_index == -1) break;
                login_user(a.fp_index>>2);
                test_user = true;
                break;
            case AT_WEBEvent:
                if (a.web.event == WE_GameEnd) return false;
                // web_event_process(a); - No any WEB editors in Game
                break;
            case AT_TouchDown: draw_login_credentials(); break;
            default: break;
        }
    }
}

static void fg_edit(int user_index)
{
    if (!(current_user.options & UO_CanEditFG)) return;

    web_options.set_fg_editor_user(user_index);
    bool new_user = (user_index == -1);
    uint8_t filled_tpls;
    MsgActivity act(AT_Fingerprint1|AT_WEBEvent|AT_WatchDog);
    act.setup_watchdog_ticks(SC_FGEditAnimSpeed);
    act.setup_web_ping_type("ping-fgedit");

    AnimatedPannel panel("Ввод отпечатков для польэователя", SizeDef{
        .max_text_length = 19, // Палец №1 - заполнен
        .max_text_lines = 3, // Max 4 lines, but no Icons in this case
        .max_icons_lines = 1,
        .max_icons_count = SC_MAX_CH
    });

//    lcd_message("Ввод отпечатков для польэователя\nЗавершение через WEB страницу");

    if (new_user)
    {
        user_index = fge_allocate_user();
        if (user_index == -1)
        {
            web_send_cmd("{'cmd':'alert', 'msg':'Лимит пользователей исчерпан (максимум 32 штуки)'}");
            restart_web_page("/web/admin.html");
            return;
        }
        do_fg_del(user_index*4, 4);
        filled_tpls = 0;
    }
    else
    {
        filled_tpls = fge_get_filled_tpls(user_index);
    }

    int tpl_to_fill = -1; // <index>*8 + <circle-index-to-fill>, or -1 if nothing to be filled

    const auto unhighlight = [&]() {
        if (tpl_to_fill == -1) return;
        web_send_cmd(R"([
            {'cmd':'fgedit-box-state','dst':%d,'state':'%s'},
            {'cmd':'fgedit-circle-state', 'dst':'%d-%d','state':'empty'}
        ])", tpl_to_fill >> 3, (filled_tpls >> (tpl_to_fill >> 3)) & 1 ? "filled" : "empty",
        tpl_to_fill >> 3, tpl_to_fill&7);
    };

    const auto msg_flash = [&](Action& a) {
        if ((a.fp_index >> 2) == user_index) // Match in me
        {
            double pc = double(a.fp_score) / int(SC_FPScope100);
            web_send_cmd("[{'cmd':'fgedit-box-msg', 'msg':'%.1f%%','dst':%d,'hlt':%f},"
                          "{'cmd':'alert', 'msg':'Уже есть такой палец'}]", 
                           pc*100, a.fp_index & 3, pc);
        }
        else // Not me
        {
            double pc = double(a.fp_score) / int(SC_FPScope100);
            web_send_cmd("{'cmd':'alert', 'msg':'Уже есть такой палец у \"%s\" - %.1f%% совпадения'}", 
                           EEPROMUserName(a.fp_index>>2).utf8(), pc*100);

        }
    };

    const char* circle_state_override = "filling";

    for(;;)
    {
        if (tpl_to_fill == -1 && filled_tpls != 15) // Not all filled - send filling indicator
        {
            // __builtin_ctz - Returns the number of trailing 0-bits in x, starting at the least significant bit position.
            tpl_to_fill = __builtin_ctz(~filled_tpls)*8;
        }
        if (tpl_to_fill != -1)
        {
            web_send_cmd(R"([
                {'cmd':'fgedit-box-state','dst':%d,'state':'filling'},
                {'cmd':'fgedit-circle-state', 'dst':%d,'state':'%s'}
            ])", tpl_to_fill >> 3, (tpl_to_fill >> 3)*SC_MAX_CH+(tpl_to_fill&7), circle_state_override);

            if ((tpl_to_fill&7) == 0) // New line of Icons
            {
                panel.body_reset();
                for(int i=0; i<4; ++i)
                {
                    if (filled_tpls & bit(i)) panel.add_text_utf8("Палец №%d - заполнен", i+1); else
                    if ((tpl_to_fill >> 3) == i) panel.add_icons(fg_icon, SC_MAX_CH, rgb(FGEDIT_ICON_COLOR_NOT_FILLED)); 
                    else panel.add_text_utf8("Палец №%d - пусто", i+1);
                }
                panel.body_draw();
            }
            panel.animate_icon(tpl_to_fill&7, FGEDIT_ICON_COLOR_FILLING_SETUP).tick(false);
        }
        circle_state_override = "filling";
        Activity::FPAccess(&act).access().active_page = (tpl_to_fill&7) + 1;
        Action a;
        for(;;)
        {
            a = act.get_action();
            if (a.type != AT_WatchDog) break;
            panel.tick();
        }
        if (a.type == AT_Fingerprint)
        {
            ESP_LOGI(TAG, "FG Editor: Got finger (tpl buffer = %d), index=%d, score=%d (msg=%s)", 
                (tpl_to_fill&7) + 1, a.fp_index, a.fp_score, a.fp_error ? a.fp_error : "");
            if (a.fp_index != -1 && a.fp_score < SC_MaxFPScope) // This is acceptable duplicate
            {
                msg_flash(a);
                a.fp_index = -1; 
                a.fp_score = R503_NO_MATCH_IN_LIBRARY;
            }
            if (a.fp_index == -1 && a.fp_score == R503_NO_MATCH_IN_LIBRARY) // This is ok
            {
                // unhighlight();
                auto cbox = tpl_to_fill >> 3;
                web_send_cmd("{'cmd':'fgedit-circle-state', 'dst':%d,'state':'filled'}", cbox*SC_MAX_CH+(tpl_to_fill&7));
                panel.animate_icon(tpl_to_fill & 7, FGEDIT_ICON_COLOR_FILLED_SETUP).tick(false);
                if ((tpl_to_fill & 7) != SC_MAX_CH-1) {++tpl_to_fill; continue;}// 1st part was filled - fill seconds

                ESP_LOGI(TAG, "FG Editor: Create FG library entry");

                tpl_to_fill = -1;

                Activity::FPAccess fpa(&act);
                auto & fp = fpa.access();
                auto err = fp.createTemplate();
                if (err)
                {
                    web_send_cmd("{'cmd':'popup','msg':'%s'}", fp.errorMsg(err));
                    ESP_LOGE(TAG, "FG Editor: CreateTemplate Error - %s", fp.errorMsg(err));
                    continue;
                }
                err = fp.storeTemplate(1, user_index*4 + cbox);
                if (err)
                {
                    web_send_cmd("{'cmd':'popup','msg':'%s'}", fp.errorMsg(err));
                    ESP_LOGE(TAG, "FG Editor: StoreTemplate Error - %s", fp.errorMsg(err));
                    continue;
                }
                filled_tpls |= 1 << cbox;
                web_send_cmd("{'cmd':'fgedit-box-state','dst':%d,'state':'filled'}", cbox);
                ESP_LOGI(TAG, "FP Editor: Box %d filled", cbox);
                continue;
            }
            if (a.fp_index == -1) // This is some error
            {
                web_send_cmd("{'cmd':'alert', 'msg':'Ошибка - %s'}", a.fp_error);
                circle_state_override = a.fp_score == R503_IMAGE_MESSY || a.fp_score == R503_FEATURE_FAIL ? "quality" : "bad";
                panel.animate_icon(tpl_to_fill&7, FGEDIT_ICON_COLOR_ERROR_SETUP, true);

            }
            else // Duplicate
            {
                msg_flash(a);
                circle_state_override = "bad";
                panel.animate_icon(tpl_to_fill&7, FGEDIT_ICON_COLOR_ERROR_SETUP, true);
                if (!filled_tpls) // but nothing was entered yet - suggest to edit another user
                {
                    web_send_cmd("{'cmd':'fgedit-switch', 'usr':'%s','usrindex':%d,'percent':%f}", 
                        EEPROMUserName(a.fp_index>>2).utf8(),
                        a.fp_index>>2,
                        double(a.fp_score) / int(SC_FPScope100));
                }
            }
        }
        else // AT_WEBEvent
        {
            switch(a.web.event)
            {
                case WE_FGDel:
                {
                    uint8_t box_idx = a.web.p1 & 3;
                    uint8_t new_filled_tpls = filled_tpls & ~(1 << box_idx);
                    unhighlight();
                    if (!new_filled_tpls) break;
                    int idx_to_delete = a.web.p1;
                    if (idx_to_delete & 0x1000) {idx_to_delete &= 3; idx_to_delete += user_index*4;}
                    do_fg_del(idx_to_delete);
                    filled_tpls = new_filled_tpls;
                    tpl_to_fill = -1;
                    web_send_cmd(R"([
                        {'cmd':'fgedit-box-state','dst':%d,'state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d','state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d','state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d','state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d','state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d','state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d','state':'empty'}
                    ])", box_idx, box_idx*SC_MAX_CH, box_idx*SC_MAX_CH+1, box_idx*SC_MAX_CH+2, box_idx*SC_MAX_CH+3, box_idx*SC_MAX_CH+4, box_idx*SC_MAX_CH+5);
                    break;
                }
                case WE_Logout: if (!new_user) {ESP_LOGI(TAG, "FG Editor - returns (no usr update)"); return;} break;
                case WE_FGE_Done: 
                    if (new_user && filled_tpls && a.web.p2) {fge_activate_user(user_index, a.web.p2, a.web.p1); free((char*)a.web.p2);}
                    ESP_LOGI(TAG, "FG Editor - FGE_Done");
                    return;
                default: break;
            }
        }
    }
}

static void fg_view()
{
    if (!(current_user.options & UO_CanViewFG)) return;

    FPLib fp_lib;
    uint32_t users = get_eeprom_users();
    Prn prn;

    MsgActivity act(AT_Fingerprint1|AT_WEBEvent);
    act.setup_watchdog(SC_TurnoffDelay);
    act.setup_web_ping_type("ping-fgview");
    fp_lib.sanitize();

    {
        Activity::FPAccess(&act).access().active_page = 1;
    }

    lcd_message("Просмотр отпечатков пальцев.\nЗавершение через WEB страницу");

    for(;;)
    {
        Action a = act.get_action();
        if (a.type & AT_Fingerprint)
        {
            prn.strcpy("[");

            bool add_comma = false;
            const auto p = [&]() -> Prn& {if (add_comma) prn.strcat(","); add_comma=true; return prn;};

            if (a.fp_index == -1 && a.fp_error)
            {
                p().cat_printf("{'cmd':'alert','msg':'Ошибка - %s'}", a.fp_error);
            }
            Activity::FPAccess fpa(&act);
            auto& fp = fpa.access();
            for(int usr_index=0; usr_index<32; ++usr_index)
            {
                if (!(users & bit(usr_index))) continue;
                uint8_t tpl_sc = fp_lib[usr_index];
                for(int tpl_subidx=0; tpl_subidx<4; ++tpl_subidx, tpl_sc >>= 1)
                {
                    if (tpl_sc & 1)
                    {
                        int tpl_index = usr_index * 4 + tpl_subidx;
                        if (fp.loadTemplate(2, tpl_index))
                        {
                            p().cat_printf("{'cmd':'fgview-box-msg','msg':'Error','dst':'%d-%d'}", tpl_index>>2, tpl_index & 3);
                            continue;
                        }
                        uint16_t score;
                        if (!fp.matchFinger(score))
                        {
                            double pc = double(score) / int(SC_FPScope100);
                            p().cat_printf("{'cmd':'fgview-box-msg','dst':'%d-%d', 'msg':'%d%%','hlt':%f}", 
                                tpl_index>>2, tpl_index & 3,
                                int(pc*100+0.5), pc);
                        }
                    }
                }
            }
            if (add_comma)
            {
                prn.strcat("]");
                web_send_cmd("%s", prn.c_str());
            }
        }
        else // WEB
        {
            switch(a.web.event)
            {
                case WE_FGDel:
                {
                    if (!(current_user.options & UO_CanEditFG)) break;
                    do_fg_del(a.web.p1);
                    restart_web_page("/web/fg_viewer.html");
                    break;
                }
                case WE_Logout: case WE_FGE_Done: ESP_LOGI(TAG, "FG Viewer returns"); return;
                case WE_FGEdit: ESP_LOGI(TAG, "FG Viewer -> FG Editor"); Activity::queue_action(a); return;
                default: break;
            }
        }
    }
}

// Draw 'Help' icon in top right corner
void draw_help_icon()
{
    Activity::LCDAccess acc(NULL);
    auto& lcd = acc.access();
    lcd.set_bg(0);
    lcd.icon32x32(RES_X-32, 0, help_icon, 0x27E8);
}

// Check if FP index belong to user that can help in opening door
bool check_open_door_fingerprint(const Action &a)
{
    assert(a.type & AT_Fingerprint);
    auto idx = a.fp_index;
    if (idx == -1) return false;
    idx >>= 2;
    if (idx == logged_in_user) return false;
    UserSetup usr;
    usr.load(idx, NULL);
    return (usr.options & UO_CanHelpUser) != 0;
}

enum ChType {
    CT_Riddle,
    CT_Expr,
    CT_Game15,
    CT_TimeGame,

    CTTotal
};
struct ChallengeSetup {
    int min_age;
    int max_age;
    int ch_percents[CTTotal];
};

static ChallengeSetup ch_setup[] = {
    {0, 10, {50, 1, 1, 1}},
    {10, 20, {50, 5, 2, 1}},
    {20, 30, {40, 5, 2, 2}},
    {30, 99, {20, 8, 3, 3}}
};

static ChType select_inside_ch_group(const ChallengeSetup& g, int exclude)
{
    int pc[CTTotal]{};
    int total=0;

    for(int i=0; i<CTTotal; ++i)
    {
        if (!(exclude & bit(i)))
        {
            total += (pc[i] = g.ch_percents[i]);
        }
    }
    if (!total) return CTTotal;
    int val = esp_random() % total;
    for(int i=0; i<CTTotal; ++i)
    {
        if (!pc[i]) continue;
        val -= pc[i];
        if (val <= 0) return ChType(i);
    }
    return CTTotal;
}

static ChType select_challenge_group(int exclude)
{
    for(const auto& g: ch_setup)
    {
        if (current_user.age >= g.min_age && current_user.age <= g.max_age)
        {
            return select_inside_ch_group(g, exclude);
        }
    }
    return select_inside_ch_group(ch_setup[0], exclude);
}

static void challenge()
{
    int exclude = 0;
    int res;
    for(;;)
    {
        ChType sel = select_challenge_group(exclude);
        switch(sel)
        {
            case CT_Riddle: res = Riddle::run_challenge(); break;
            case CT_Expr:   res = EQuest::run_challenge(); break;
            case CT_Game15: res = Game15::run_challenge(); break;
            case CT_TimeGame: res = TileGame::run_challenge(); break;
            default: res = CR_Ok; break;
        }
        if (res == CR_Error) exclude |= bit(sel); 
        else break;
    }
    if (res == CR_Timeout) {passivate(); return;}

    int door_idx = working_state.get_loaded_gift(logged_in_user);
    assert(door_idx != -1);
    auto open_door_res = open_door(door_idx);
    if (open_door_res & ODR_Opened) working_state.unload_gift(door_idx);
    if (open_door_res & ODR_Timeout) {passivate(); return;}
    if (open_door_res & ODR_Login) 
    {
        Activity::queue_action(Action{.type = AT_Fingerprint, .fp_index = int16_t(open_door_res&31)});
    }
    logged_in_user = -1;
}


} // namespace Interactive
