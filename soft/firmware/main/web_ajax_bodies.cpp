#include "common.h"
#include "hadrware.h"
#include "setup_data.h"
#include "web_vars.h"
#include "challenge_mgr.h"
#include "activity.h"
#include "bg_image.h"

#include "web_ajax_classes.h"

static const char TAG[] = "ajax";

// gift_load.html - Load gift to user + door
//
// Params: 
//     door - Door index (0 based)
//     user - User index (0 based)
// Return (text):
//     User name to put on Door (with serial number)

// G(gift_load, P2(I, door, I, user))
void AJAXDecoder_gift_load::run()
{
    sol_hit(arg_door);
    int total = working_state.total_loaded_gift(arg_user);
    working_state.load_state[arg_door] = (total << 5) | arg_user;
    working_state.sync();

    UserSetup usr;
    if (usr.load(arg_user, NULL) && (usr.status & US_Done))
    {
        usr.status &= ~US_Done;
        usr.save(arg_user, NULL);
    }

    if (!working_state.write_user_name(*this, arg_door, false)) *this << UTF8 << "---";
}

// unload_gift.html - Remove gift from specified door
// 
// Params:
//     door - Door index (0 based)
// Return (JSON):
//     Array of 8 text (or null) elements with titles for Doors
 
// G(unload_gift, P1(I, door))
void AJAXDecoder_unload_gift::run()
{
//    sol_hit(arg_door);
    working_state.unload_gift(arg_door);
    working_state.sync();
    web_options.LoadedGiftDoors(*this);
}

// open_door.html - Open Door
// 
// Params:
//     door - Door index (0 based)

// G(open_door, P1(I, door))
void AJAXDecoder_open_door::run()
{
    sol_hit(arg_door);
    *this << UTF8 << "Ok";
}

// done_user.html - Mark user as Done
// 
// Param:
//     user - User index (or -1)
// Return (JSON):
//     Aray of 2 items: HTML text with list of still Active users, HTML text with list of Done users

// G(done_user, P1(I, user))
void AJAXDecoder_done_user::run()
{
    if (arg_user != -1)
    {
        if (working_state.total_loaded_gift(arg_user)) // We can't terminate this user - so send an erorr
        {
            *this << UTF8 << "[\"<span class='error'>ÐÐµ Ð¼Ð¾Ð³Ñƒ Ð¾Ñ‚ÐºÐ»ÑŽÑ‡Ð¸Ñ‚ÑŒ Ð¿Ð¾Ð»ÑŒÐ·Ð¾Ð²Ð°Ñ‚ÐµÐ»Ñ:<br>Ð£ Ð½ÐµÐ³Ð¾ ÐµÑÑ‚ÑŒ Ð·Ð°Ð³Ñ€ÑƒÐ¶ÐµÐ½Ð½Ñ‹Ðµ Ð¿Ð¾Ð´Ð°Ñ€ÐºÐ¸ - Ð¾Ð½ ÑÐ½Ð°Ñ‡Ð°Ð»Ð° Ð´Ð¾Ð»Ð¶ÐµÐ½ Ð¸Ñ… Ð·Ð°Ð±Ñ€Ð°Ñ‚ÑŒ</span>\", \"\"]";
            return;
        }
        UserSetup usr; 
        usr.load(arg_user, NULL);
        usr.status |= US_Done;
        usr.save(arg_user, NULL);
    }
    *this << UTF8 << "[\"" ;
    web_options.write_active_users(*this);
    *this << UTF8 << "\",\"";
    web_options.write_done_users(*this);
    *this << UTF8 << "\"]" ;
}

// set_interround_time.html - Set interround time (in minutes)
// 
// Param:
//     value - Time

// G(set_interround_time, P1(I, value))
void AJAXDecoder_set_interround_time::run()
{
    global_setup.round_time = arg_value;
    global_setup.sync();
    *this << UTF8 << "Ok";
}

// del_challenge.html - Delete challenge
// 
// Param:
//     index - Index of challenge

// G(del_challenge, P1(I, index))
void AJAXDecoder_del_challenge::run()
{
    challenge_mgr().delete_challenge(arg_index);
    *this << UTF8 << "Ok";
}

// get_challenge.html - Load challenge contents
// 
// Param:
//     index - Index of challenge
// Result: text with challenge definition

// G(get_challenge, P1(I, index))
void AJAXDecoder_get_challenge::run()
{
    challenge_mgr().send_challenge(*this, arg_index);
}


#define Q "\""
#define ONAME(n) Q #n Q ":"

// get_user_opts.html - get User definition
// 
// Param:
//     index - user index
// Result (JSON):
//     Object with definition:
//         name   : str - Name of User
//         age    : int - Age os User
//         prio   : int - Priority of User
//         disable: bool - 'disabled' status of user
//         rights : int - Scale of user rights

// G(get_user_opts, P1(I, index))
void AJAXDecoder_get_user_opts::run()
{
    UserSetup usr;
    uint8_t name[33];
    usr.load(arg_index, name);
    *this << UTF8 << "{" 
        << ONAME(name)      << Q << DOS << name << UTF8 << Q << ","
        << ONAME(age)       << usr.age << ","
        << ONAME(prio)      << usr.priority << ","
        << ONAME(disable)   << (usr.status & US_Enabled ? "false" : "true") << ","
        << ONAME(rights)    << usr.options << "}";
}

// set_user_opt.html - Set User definition field
// 
// Params:
//     index - User Index
//     name  - Name of field (see get-user-opts.html result JSON)
//     value - Value for this field
// Return (string):
//     User name for User List

// G(set_user_opt, P3(I, index, SU, name, SD, value))
void AJAXDecoder_set_user_opt::run()
{
    UserSetup usr;
    uint8_t name[33];
    usr.load(arg_index, name);

    switch(arg_name[0])
    {
        case 'n': strncpy((char*)name, arg_value, 32); name[32]=0; break;
        case 'a': usr.age = atoi(arg_value); break;
        case 'p': if (logged_in_user != arg_index) usr.priority = std::min<uint32_t>(current_user.priority, atoi(arg_value)); break;
        case 'd': 
            if (!(current_user.options & UO_CanDisableUser)) break;
            if (arg_value[0] == 't') usr.status &= ~US_Enabled; else usr.status |= US_Enabled; 
            break;
        case 'r': 
            if (logged_in_user != arg_index)
                {
                    uint32_t opts = atoi(arg_value);
                    opts &= current_user.options;
                    opts |= ~current_user.options & usr.options;
                    usr.options = opts; break;
                }
            break;
        default: break;
    }
    usr.save(arg_index, name);
    usr.write_usr_name(*this, name);
}

// del_user.html - Delete user
// 
// Params:
//     index - User Index

// G(del_user, P1(I, index))
void AJAXDecoder_del_user::run()
{
    UserSetup usr;
    uint8_t name[32] = {};
    usr.clear();
    usr.save(arg_index, name);
    Activity::FPAccess(NULL).access().deleteTemplate(arg_index*4, 4);
    *this << UTF8 << "Ok";
}

// del_user_fg.html - Delete user fingerprint from library
// 
// Params:
//     usr_index - User index (or -1 for user currently on edit)
//     fg_index - FG index

// G(del_user_fg, P2(I, usr_index, I, fg_index))
void AJAXDecoder_del_user_fg::run()
{
//    Activity::FPAccess(NULL).access().deleteTemplate(arg_usr_index*4 + arg_fg_index, 1);
    int index = arg_usr_index == -1 ? (0x1000 | arg_fg_index) : arg_usr_index*4 + arg_fg_index;
    Activity::queue_action(Action{.type=AT_WEBEvent, .web={.event=WE_FGDel, .p1=index}});

    *this << UTF8 << "Ok";
}


static const char* last_ping_tag;
static int ping_count = SC_PingTimeout;

// ping.html - Answer for ping command
// 
// Params:
//     id  - Ping command (ping-fgview or ping-fgedit)
//     cnt - Counter from command

// G(ping, P2(SU, id, I, cnt))
void AJAXDecoder_ping::run()
{
    if (last_ping_tag && strcmp(last_ping_tag, arg_id)==0) ping_count = SC_PingTimeout;
    // counter is ignored
    *this << UTF8 << "Ok";
}

void send_web_ping_to_ws(const char* tag)
{
    static int rcount;
    if (tag != last_ping_tag) // We use only constant strings with 'tag', so we can compare pointers here
    {
        last_ping_tag = tag;
        ping_count = SC_PingTimeout;
    }
    web_send_cmd("{'cmd':'%s','cnt':%d}", tag, rcount++);
    if (ping_count && !--ping_count) // We got timeout
    {
        ping_count = SC_PingTimeout;
        Activity::queue_action(Action{.type = AT_WEBEvent, .web={.event=WE_Logout}});
    }
}

// Enable/Disable users to participate in game
static void setup_active_users(uint32_t scale)
{
    bool ws_updated = false;
    for(int i=0; i<32; ++i)
    {
        UserSetup usr;
        if (!usr.load(i, NULL)) continue;
        if (!(usr.status & US_Enabled)) continue;
        bool req_enabled = (scale & bit(i)) != 0;
        bool has_enabled = (usr.status & US_Paricipated) != 0;
        if (req_enabled != has_enabled)
        {
            usr.status ^= US_Paricipated;
            usr.save(i, NULL);
            if (req_enabled) // Turn on 'gift-right-now' for new user
            {
                working_state.enabled_users |= bit(i);
                ws_updated = true;
            }
        }
    }
    if (ws_updated) working_state.sync();
}

// update_users.html - Update list of Users which participated in Game.
// -> load_gifts.html or admin.html
// 
// Params:
//     u<N> - Flag 'User take a part in game'
//     load_gifts [opt] - Jump to load_gifts.html after processing (otherwise jump to admin.html)

// G(update_users, P2(U, users, OV, load_gifts))
void AJAXDecoder_update_users::run()
{
    setup_active_users(arg_users);
    if (arg_load_gifts) redirect("/web/load_gifts.html");
    else redirect("/web/admin.html");
}

// end_game.html - End game
// -> admin.html

// G(end_game, P0)
void AJAXDecoder_end_game::run()
{
    working_state.state = WS_NotActive;
    working_state.sync();
    Activity::queue_action(Action{.type = AT_WEBEvent, .web={.event=WE_GameEnd}});
    redirect("/web/admin.html");
}

// start_game.html - Start game
// -> admin.html
// 
// Optional params (if not exists - do not change current selection):
//     u<N> - Flag 'User take a part in game'
//     start_time - Pending time

// G(start_game, P2(U, users, OSU, start_time))
void AJAXDecoder_start_game::run()
{
    if (arg_users) setup_active_users(arg_users);
    if (arg_start_time && *arg_start_time) // This is pending game
    {
        char* e;
        auto now = time(NULL);
        auto stm = *gmtime(&now);

        stm.tm_sec = 0;
        stm.tm_hour = strtoul(arg_start_time, &e, 10);
        stm.tm_min = strtoul(e+1, NULL, 10);

        auto target = mktime(&stm);
        if (target < now) target += 24*60*60;

        working_state.last_round_time = utc2ts(target);
        working_state.state = WS_Pending;
        working_state.sync();
    }
    else
    {
        start_game();
        Activity::queue_action(Action{.type = AT_WEBEvent, .web={.event=WE_GameStart}});
    }
    redirect("/web/admin.html");
}

// add_user.html - Add new user (no name). Pass through fingerprint addition, than jump to edit_user.html
// -> /action/fg_edit.html (no params) -> edit_user.html

// G(add_user, P0)
void AJAXDecoder_add_user::run()
{
//    Activity::push_action(Action{.type = AT_WEBEvent, .web={.event=WE_FGEdit, .p1=-1}});
    redirect("/action/fg_edit.html");
}

// bg_add.html (POST as binary) - Add new background image
// 
// Param - blob with JPG image
// Answer - index of added BG image (or -1 if can't add)

// S(bg_add)
size_t AJAXDecoder_bg_add::consume_stream(uint8_t* data, size_t size, bool eof)
{
    if (!data) // Start or end of stream
    {
        if (!eof) // start
        {
            opaque_2 = bg_images.create_new_bg_image();
            opaque_1 = bg_images.open_image(opaque_2, "w"); // Check for opaque_1 value for NULL
        }
        else
        {
            if (opaque_1) fclose(opaque_1);
            *this << (opaque_1 ? opaque_2 : -1);
        }
    }
    else if (opaque_1)
    {
        return fwrite(data, 1, size, opaque_1);
    }
    return size;
}

// bg_remove.html - Remove BG image
// -> set_bg_images.html
// 
// Param:
//     index - BG image index

// G(bg_remove, P1(I, index))
void AJAXDecoder_bg_remove::run()
{
    bg_images.delete_bg_image(arg_index);
    redirect("/web/set_bg_images.html");
}

// recalibrate_touch.html - Recalibrate touch pannel
// -> setup.html

// G(recalibrate_touch, P0)
void AJAXDecoder_recalibrate_touch::run()
{
    Activity act(AT_TouchDown|AF_Override);
    Activity::LCDAccess a(&act);
    touch_setup.calibrate();
    redirect("/web/setup.html");
}

// reset.html - Restart system

// G(reset, P0)
void AJAXDecoder_reset::run()
{
    reboot();
    *this << UTF8 << "System restarts in 5 seconds";
}

// zap.html - Erase whole EEPROM, Fingerprints and restart

// G(zap, P0)
void AJAXDecoder_zap::run()
{
    zap_configs();
    reboot();
    *this << UTF8 << "System restarts in 5 seconds";
}

// fg_view.html - Viewer FingerPrints library
// -> fg_viewer.html

// G(fg_view, P0)
void AJAXDecoder_fg_view::run()
{
    Activity::queue_action(Action{.type = AT_WEBEvent, .web={.event=WE_FGView}});
    redirect("/web/fg_viewer.html");
}

// fg_edit.html -> Editor FingerPrints library for user
// -> fg_editor.html
// 
// Param:
//     index [opt] - User to edit. If ommited - reserve new User

// G(fg_edit, P1(OI, index))
void AJAXDecoder_fg_edit::run()
{
    int idx = arg_index ? *arg_index : -1;
    ESP_LOGI(TAG, "FG Edit action: idx=%d", idx);
    web_options.set_fg_editor_user(idx);
    Activity::queue_action(Action{.type = AT_WEBEvent, .web={.event=WE_FGEdit, .p1=idx}});
    redirect("/web/fg_editor.html");
}

// fg_viewer_done.html - Exit from Viewer FingerPrints library
// -> setup.html

// G(fg_viewer_done, P0)
void AJAXDecoder_fg_viewer_done::run()
{
    Activity::queue_action(Action{.type = AT_WEBEvent, .web={.event=WE_Logout}});
    redirect("/web/setup.html");
}

// fg_editor_done.html - Exit from Editor FingerPrints
// -> edit_user.html
// 
// Param:
//     name [opt] - Name for new User
//     age [opt] - Age of new user

// G(fg_editor_done, P2(OSD, name, OI, age))
void AJAXDecoder_fg_editor_done::run()
{
    // p1 - new user age (or -1), p2 - new user name (DOS) or NULL
    int p1 = arg_age ? *arg_age : -1;
    Activity::push_action(Action{.type = AT_WEBEvent, .web={.event=WE_FGE_Done, .p1=p1, .p2=arg_name ? strdup(arg_name) : NULL}}); //!!! Possible Memory leak.
    redirect("/web/edit_user.html");
}

// update_challenge.html (POST as text) - Update challenge
// 
// Param - challenge text (with 'i' tag prepended)
// Answer - index of challenge (or -1 if challenge save/creation failed)

// S(update_challenge)
size_t AJAXDecoder_update_challenge::consume_stream(uint8_t* data, size_t size, bool eof)
{
    if (!data)
    {
        if (eof && opaque_1) {fclose(opaque_1); *this << opaque_2;}
        return size;
    }
    int delta=0;
    if (!eof)
    {
        auto new_size = valid_utf8_size((const char*)data, size);
        delta = size - new_size;
        size = new_size;
    } 

    auto out_size = utf8_to_dos((char*)data, size);
    if (!opaque_1) // First entry - open file and write first pack to it directly by Challenge Manager
    {
        auto def = challenge_mgr().update_challenge(data, out_size);
        opaque_1 = def.file;
        opaque_2 = def.ch_index;
        return size - delta;
    }
    fwrite(data, 1, out_size, opaque_1);
    return size - delta;
}
