#include "common.h"
#include "setup_data.h"
#include "activity.h"
#include "prnbuf.h"
#include "text_draw.h"
#include "bg_image.h"
#include "web_vars.h"
#include "web_gadgets.h"

BGImage bg_images;

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

// Activate (create) new user. 'name' in UTF8
static void fge_activate_user(int user_index, const char* name, int age)
{
    if (!(current_user.options & (UO_CanAddRemoveUser|UO_CanAddRemoveAdmin))) return;
    char b[66]; // UTF8 string can be 2x length of DOS version (in Russian CP) + 1 extra symbol for compensate possible half of last symbol
    strncpy(b, name, sizeof(b)-1);
    b[65] = b[64] = 0;
    UserSetup usr = current_user;
    if (!(current_user.options & UO_CanAddRemoveAdmin)) usr.options = 0;
    usr.age = age;
    utf8_to_dos(b);
    usr.save(user_index, (uint8_t*)b);
}

// Draw message (in UTF8) to LCD in centered Box
static void lcd_message(const char* msg, ...)
{
    static TextBoxDraw::TextGlobalDefinition gd;
    static Prn buf;
    TextBoxDraw::TextsParser pars(gd);
    va_list l;

    if (msg)
    {
        va_start(l, msg);
        buf.vprintf(msg, l);
        va_end(l);
    }

    utf8_to_dos(buf.c_str());
    pars.parse_text(buf.c_str());
    pars.draw_one_box_centered(Activity::LCDAccess(NULL).access());
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
inline void set_web_message(const char* title, const char* message) {web_options.set_title_and_message(title, message);}

// Delete FG. index is <User-index>*4 + <FG-index-in-lib>
inline void do_fg_del(int index, uint8_t count=1)
{
    if (current_user.options & UO_CanEditFG) Activity::FPAccess(NULL).access().deleteTemplate(index, count);
    // else - send error as AJAX result
}

static void game();
static void no_game();
static void challenge();

static void fg_edit(int user_index);
static void fg_view();

void entry()
{
    bg_images.peek_next_bg_image();
    bg_images.draw(Activity::LCDAccess(NULL).access());

    switch(working_state.state)
    {
        case WS_Pending: lcd_message("Приходите в\n", ts_to_string(working_state.last_round_time)); no_game(); return;
        case WS_Active: game(); return;
        //case WS_NotActive: 
        default: break;
    }
    if (override_switch_active() || !get_eeprom_users())
    {
        login_superuser();
        set_web_root("web/admin.html");
        lcd_message("Ждите настройки");
        do {MsgActivity(AT_WEBEvent).get_action();} while(working_state.state == WS_NotActive && (override_switch_active() || !get_eeprom_users()));
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
}

static void web_event_process(const Action& act)
{
    switch(act.web.event)
    {
        case WE_FGDel: do_fg_del(act.web.p1); break;
        case WE_FGEdit: fg_edit(act.web.p1); break;
        case WE_FGView: fg_view(); break;
        default: break;
    }
}

static void no_game()
{
    MsgActivity act(AT_WatchDog|AT_Fingerprint0|(working_state.state == WS_Pending ? AT_Alarm : 0));
    if (working_state.state == WS_Pending) act.setup_alarm_action(ts2utc(working_state.last_round_time));
    act.setup_watchdog(SC_TurnoffDelay);

    for(;;)
    {
        Action a = act.get_action();
        switch(a.type)
        {
            case AT_WatchDog: passivate(); return;
            case AT_Fingerprint:
            {
                if (a.fp_index == -1) continue;
                login_user(a.fp_index>>2);
                if (!current_user.options) continue;
                lcd_message("Настройка системы Админом");
                restart_web_page("/");
                auto act = wait(AT_WEBEvent);
                switch(act.web.event)
                {
                    case WE_Logout: return;
                    case WE_GameStart: game(); return;
                    case WE_FGDel: web_event_process(act); break;
                    case WE_FGEdit: case WE_FGView: web_event_process(act); return;
                    default: break;
                }
                break;
            }
            case AT_Alarm: game(); return;
            default: break;
        }        
    }
}

static void game()
{
    MsgActivity act(AT_WatchDog|AT_Fingerprint|AT_WEBEvent);
    act.setup_watchdog(SC_TurnoffDelay);
    bool test_user = (logged_in_user >= 0);

    for(;;)
    {
        const auto round = global_setup.round_time*60;
        uint32_t now = utc2ts(time(NULL));
        if (working_state.last_round_time + round >= now)
        {
            working_state.enabled_users = -1;
            working_state.last_round_time += ((now - working_state.last_round_time + round - 1) / round) * round;
            working_state.sync();
        }
        if (test_user)
        {
            test_user = false;
            if (current_user.options) set_web_root("web/admin.html"); 
            else set_web_message("Access denied", "У вас недостаточно прав для входа сюда");
            if ((current_user.status & (US_Enabled|US_Paricipated)) != (US_Enabled|US_Paricipated))
            {
                lcd_message("Вы не принимаете участия в\nраздаче подарков");
                continue;
            }
            if (current_user.status & US_Done)
            {
                lcd_message("Вам уже вручили все подарки");
                continue;
            }
            if (!((working_state.enabled_users >> logged_in_user) & 1))
            {
                lcd_message("Вы уже получили свой подарок\nПриходите %s за следующим", ts_to_string(working_state.last_round_time + round));
                continue;
            }
            if (working_state.get_loaded_gift(logged_in_user) == -1)
            {
                lcd_message("Для вас не загрузили подарка\nСрочно поэовите Админа");
                continue;
            }
            lcd_message("Вы готовы получить подарок?\nНо сначала загадка...");
            for(;;)
            {
                MsgActivity act(AT_WatchDog|AT_Fingerprint2|AT_WEBEvent|AT_TouchDown|AF_Override);
                act.setup_watchdog(SC_TurnoffDelay);
                Action a = act.get_action();
                switch(a.type)
                {
                    case AT_WatchDog: passivate(); return;
                    case AT_Fingerprint: 
                        if (a.fp_index == -1 || logged_in_user == a.fp_index>>2) break;
                        login_user(a.fp_index>>2);
                        test_user = true;
                        break;
                    case AT_WEBEvent:
                        if (a.web.event == WE_GameEnd) return;
                        web_event_process(a);
                        break;
                    default: break;
                }
                if (test_user) continue;
                challenge();
                return;
            }
        }
        Action a = act.get_action();
        switch(a.type)
        {
            case AT_WatchDog: passivate(); return;
            case AT_Fingerprint: 
                if (a.fp_index == -1) break;
                login_user(a.fp_index>>2);
                test_user = true;
                break;
            case AT_WEBEvent:
                if (a.web.event == WE_GameEnd) return;
                web_event_process(a);
                break;
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
    MsgActivity act(AT_Fingerprint1|AT_WEBEvent);
    act.setup_watchdog(SC_TurnoffDelay);
    act.setup_web_ping_type("ping-fgedit");
    lcd_message("Ввод отпечатков для польэователя\nЗавершение через WEB страницу");

    if (new_user)
    {
        user_index = fge_allocate_user();
        if (user_index == -1)
        {
            web_send_cmd("{'cmd':'alert', 'msg':'Лимит пользователей исчерпан (максимум 32 штуки)'}");
            restart_web_page("web/admin.html");
            return;
        }
        do_fg_del(user_index, 4);
        filled_tpls = 0;
    }
    else
    {
        filled_tpls = fge_get_filled_tpls(user_index);
    }

    int tpl_to_fill = -1; // <index>*2 + <is-second-circle-to-fill>, or -1 if nothing to be filled

    const auto unhighlight = [&]() {
        if (tpl_to_fill == -1) return;
        web_send_cmd(R"([
            {'cmd':'fgedit-box-state','dst':%d,'state':'%s'},
            {'cmd':'fgedit-circle-state', 'dst':'%d-%d','state':'empty'}
        ])", tpl_to_fill >> 1, (filled_tpls >> (tpl_to_fill >> 1)) & 1 ? "filled" : "empty",
        tpl_to_fill >> 1, tpl_to_fill&1);
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
            tpl_to_fill = __builtin_ctz(~filled_tpls)*2;
        }
        if (tpl_to_fill != -1)
        {
            web_send_cmd(R"([
                {'cmd':'fgedit-box-state','dst':%d,'state':'filling'},
                {'cmd':'fgedit-circle-state', 'dst':'%d-%d','state':'%s'}
            ])", tpl_to_fill >> 1, tpl_to_fill >> 1, tpl_to_fill&1, circle_state_override);
        }
        circle_state_override = "filling";
        Activity::FPAccess(&act).access().active_page = (tpl_to_fill&1) + 1;
        Action a = act.get_action();
        if (a.type == AT_Fingerprint)
        {
            if (a.fp_index != -1 && a.fp_score < SC_MaxFPScope) // This is acceptable duplicate
            {
                msg_flash(a);
                a.fp_index = -1; 
                a.fp_score = R503_NO_MATCH_IN_LIBRARY;
            }
            if (a.fp_index == -1 && a.fp_score == R503_NO_MATCH_IN_LIBRARY) // This is ok
            {
                unhighlight();
                if (!(tpl_to_fill & 1)) {tpl_to_fill |= 1; continue;} // 1st part was filled - fill second

                auto cbox = tpl_to_fill >> 1;
                tpl_to_fill = -1;

                Activity::FPAccess fpa(&act);
                auto & fp = fpa.access();
                auto err = fp.createTemplate();
                if (err)
                {
                    web_send_cmd("{'cmd':'popup','msg':'%s'}", fp.errorMsg(err));
                    continue;
                }
                err = fp.storeTemplate(1, user_index*4 + cbox);
                if (err)
                {
                    web_send_cmd("{'cmd':'popup','msg':'%s'}", fp.errorMsg(err));
                    continue;
                }
                filled_tpls |= 1 << cbox;
                web_send_cmd(R"([
                    {'cmd':'fgedit-box-state','dst':%d,'state':'filled'},
                    {'cmd':'fgedit-circle-state', 'dst':'%d-%d','state':'filled'},
                    {'cmd':'fgedit-circle-state', 'dst':'%d-%d','state':'filled'}
                ])", cbox, cbox, cbox);
                continue;
            }
            if (a.fp_index == -1) // This is some error
            {
                web_send_cmd("{'cmd':'alert', 'msg':'Ошибка - %s'}", a.fp_error);
                circle_state_override = a.fp_score == R503_IMAGE_MESSY || a.fp_score == R503_FEATURE_FAIL ? "quality" : "bad";
            }
            else // Duplicate
            {
                msg_flash(a);
                circle_state_override = "bad";
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
                        {'cmd':'fgedit-circle-state', 'dst':'%d-0','state':'empty'},
                        {'cmd':'fgedit-circle-state', 'dst':'%d-1','state':'empty'}
                    ])", box_idx, box_idx, box_idx);
                    break;
                }
                case WE_Logout: return;
                case WE_FGE_Done: 
                    if (new_user && filled_tpls && a.web.p2) fge_activate_user(user_index, a.web.p2, a.web.p1);
                    return;
                default: break;
            }
        }
    }
}

static void fg_view()
{
    if (!(current_user.options & UO_CanViewFG)) return;

    uint8_t buf[32];
    uint32_t users = get_eeprom_users();
    Prn prn;

    MsgActivity act(AT_Fingerprint1|AT_WEBEvent);
    act.setup_watchdog(SC_TurnoffDelay);
    act.setup_web_ping_type("ping-fgview");

    {
        Activity::FPAccess fpa(&act);
        auto& fp = fpa.access();
        int err = fp.readIndexTable(buf);
        if (err) return;// this is error, what to do?
        fp.active_page = 1;
    }

    lcd_message("Просмотр отпечатков пальцев\nЗавершение через WEB страницу");

    for(;;)
    {
        Action a = act.get_action();
        if (a.type & AT_Fingerprint)
        {
            prn.strcat("[");

            bool add_comma = false;
            const auto p = [&]() -> Prn& {if (add_comma) prn.strcat(","); add_comma=true; return prn;};

            if (a.fp_index == -1 && a.fp_error)
            {
                p().cat_printf("{'cmd':'alert','msg':'Ошибка - %s'}", a.fp_error);
            }
            Activity::FPAccess fpa(&act);
            auto& fp = fpa.access();
            for(int outer_idx = 0; outer_idx < 16; ++outer_idx)
            {
                uint8_t fps = buf[outer_idx];
                if (!fps) continue;
                int from, to;
                switch((users >> (outer_idx*2)) & 3)
                {
                    case 0: continue;
                    case 1: from = 0; to = 4; break;
                    case 2: from = 4; to = 8; break;
                    default: from = 0; to = 8; break;
                }
                for(int i=from; i<to; ++i)
                {
                    if ((fps >> i) & 1)
                    {
                        int tpl_index = outer_idx * 8 + i;
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
                web_send_cmd(prn.c_str());
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
                    restart_web_page("web/fg_viewer.html");
                    break;
                }
                case WE_Logout: case WE_FGE_Done: return;
                case WE_FGEdit: web_send_cmd("{'cmd':'goto','href':'act/fg_edit.html?index=%d'}", a.web.p1); return;
                default: break;
            }
        }
    }
}


} // namespace Interactive
