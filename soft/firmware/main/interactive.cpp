#include "common.h"
#include "setup_data.h"
#include "activity.h"
#include "prnbuf.h"
#include "text_draw.h"

namespace Interactive {

// Draw message (in UTF8) to LCD in centered Box
static void lcd_message(const char* msg, ...)
{
    static TextBoxDraw::TextGlobalDefinition gd;
    Prn buf;
    TextBoxDraw::TextsParser pars(gd);
    va_list l;

    va_start(l, msg);
    buf.vprintf(msg, l);
    va_end(l);

    utf8_to_dos(buf.c_str());
    pars.parse_text(buf.c_str());
    pars.draw_one_box_centered(Activity::LCDAccess(NULL).access());
}

// Turn off LCD backlight
inline void lcd_teardown() {fade_out();}
// Turn on LCD background
inline void lcd_turnon() {fade_in();}

// November 1, 2023 0:00:00
static constexpr time_t timestamp_shift = 1698796800ull;

// Convert time stamp to UTC time
inline time_t ts2utc(uint32_t tm) {return tm+timestamp_shift;}
// Convert UTC to timestamp
inline uint32_t utc2ts(time_t ts) {return ts-timestamp_shift;}

// Convert time stamp value to text (in UTF8)
static const char* ts_to_string(uint32_t tm)
{
    time_t ts = ts2utc(tm) + global_setup.tz_shift*(15*60);
    char* result = asctime(gmtime(&ts));
    if (char* e=strchr(result, '\n')) *e=0;
    return result;
}

// Setup WEB root page
static void set_web_root(const char*);
// Restart WEB page
static void restart_web_page(const char* url);

// Setup WEB root as Mesage with 'message' and 'title'
static void set_web_message(const char* title, const char* message);

// Delete FG. index is <User-index>*4 + <FG-index-in-lib>
inline void do_fg_del(int index)
{
    if (current_user.options & UO_CanEditFG) Activity::FPAccess(NULL).access().deleteTemplate(index);
    // else - send error as AJAX result
}

static void game();
static void no_game();
static void challenge();

static void fg_edit(int user_index);
static void fg_view();

void entry()
{
    switch(working_state.state)
    {
        case WS_Pending: lcd_message("Приходите в\n", ts_to_string(working_state.last_round_time)); no_game(); return;
        case WS_Active: game(); return;
        //case WS_NotActive: 
        default: break;
    }
    if (override_switch_active() || !total_users())
    {
        login_superuser();
        set_web_root("web/admin.html");
        lcd_message("Ждите настройки");
        do {Activity(AT_WEBEvent).get_action();} while(working_state.state == WS_NotActive && (override_switch_active() || !total_users()));
        return;
    }
    set_web_message("Waiting for login", "Пожалуста, войдите в систему с помощью отпечатка пальца");
    lcd_message("Раздача подарков ещё не начатаб ждите");
    no_game();
}

static Action wait(uint32_t action)
{
    return Activity(AF_Override | action).get_action();
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
    Activity act(AT_WatchDog|AT_Fingerprint0|(working_state.state == WS_Pending ? AT_Alarm : 0));
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
                login_user(a.fp_index);
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
    Activity act(AT_WatchDog|AT_Fingerprint|AT_WEBEvent);
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
            challenge();
            return;
        }
        Action a = act.get_action();
        switch(a.type)
        {
            case AT_WatchDog: passivate(); return;
            case AT_Fingerprint: 
                if (a.fp_index == -1) break;
                login_user(a.fp_index);
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


} // namespace Interactive
