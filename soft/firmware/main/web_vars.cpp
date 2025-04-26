#include "common.h"

#include "web_vars.h"
#include "setup_data.h"
#include "hadrware.h"
#include "bg_image.h"
#include "challenge_mgr.h"
#include "activity.h"
#include "log_control.h"

static const char* TAG = "web-vars";

WebOptions web_options;

void WebOptions::err_novar(const char* var) 
{
    ESP_LOGE(TAG, "WEB variable '%s' doesn't exists", var);
}

void  WebOptions::err_type_wrong(const char* var) 
{
    ESP_LOGE(TAG, "WEB variable '%s' has wrong type (should be int)", var);
}

uint8_t WebOptions::GameStarted()
{
  return working_state.state == WS_Active ? 1 : 2;
}

void WebOptions::CurrentUser(Ans &ans)         // Name of currently logged on user
{
    ans << DOS << (char*)current_user_name;
}

static void write_podarok(Ans& ans, int t)
{
    ans << UTF8 << "подар";
    switch(t)
    {
      case 1: ans << UTF8 << "ок"; break;
      case 2: case 3: case 4: ans << UTF8 << "ка"; break;
      default: ans << UTF8 << "ков"; break;
    }
}

void WebOptions::MainStatus(Ans &ans)          // HTML block with current status of system
{
    ans << UTF8 << "Игра ";
    switch(working_state.state)
    {
      case WS_NotActive: ans << UTF8 << "не начата"; break;
      case WS_Pending: ans << UTF8 << "запланированна в " << ts_to_string(working_state.last_round_time); break;
      case WS_Active: ans << UTF8 << "идёт"; break;
    }
    ans << UTF8 << "<br>";
    if (working_state.state == WS_Active) ans << UTF8 << "Следующий раунд в " << ts_to_string(working_state.last_round_time + global_setup.round_time*60) << "<br>";
    uint32_t all_users = get_eeprom_users(US_Enabled|US_Paricipated, US_Enabled|US_Paricipated);
    uint32_t done = get_eeprom_users(US_Enabled|US_Paricipated|US_Done, US_Enabled|US_Paricipated|US_Done);
    if (!all_users) ans << UTF8 << "Пользователи на игру не назначены"; else
    {
      ans << UTF8 << "Принимают участие:<br><ul>";
      for(int i=0; i<32; ++i)
      {
        if (all_users & bit(i))
        {
          ans << DOS << "<li> " << EEPROMUserName(i).dos();
          if (working_state.state != WS_Active) continue;
          ans << UTF8 << " - ";
          if (done & bit(i)) ans << UTF8 << "завершён"; else
          {
            int t = working_state.total_loaded_gift(i);
            if (!t) ans << UTF8 << "подарков не загружено";
            else {ans << UTF8 << "загружено " << t << " "; write_podarok(ans, t);}
          }
          ans << UTF8 << " - " << (working_state.enabled_users & bit(i) ? "может забрать подарок" : "подарок в следующий раз");
        }
      }
      ans << UTF8 << "</ul>";
    }
}

/*
   <option value="1">Пользователь 1</option>
   <option value="2">Пользователь 2</option>
   <option value="3">Пользователь 3</option>
   <option value="4">Пользователь 4</option>
*/
void WebOptions::HTMLUserList(Ans &ans)        // HTML block with all Users available to edit by current
{
  uint8_t name[33];

  if (!(current_user.options & UO_CanEditUser)) return;

  for(int uidx = 0; uidx < 32; ++uidx)
  {
    UserSetup usr; ;
    if (usr.load(uidx, name) && usr.priority <= current_user.priority)
    {
      ans << UTF8 << "<option value='" << uidx << "'>";
      usr.write_usr_name(ans, name);
      ans << UTF8 << "</option>";
    }
  }
}

uint16_t WebOptions::UserRights()          // Rights of currently logged-in user
{
    return current_user.options;
}

uint8_t WebOptions::CurUserPrio()         // Priority value of Current User
{
    return current_user.priority;
}

uint8_t WebOptions::TimeToDoorsEnable()   // Time to reenargizing Door system
{
    return time_to_reengage_sol;
}

void WebOptions::write_done_users(Ans &ans)
{
    uint32_t done = get_eeprom_users(US_Enabled|US_Paricipated|US_Done, US_Enabled|US_Paricipated|US_Done);
    if (!done) return;
    ans << UTF8 << "Завершили:<br><ul>";
    for(int i=0; i<32; ++i)
    {
      if (done & bit(i))
        {
          ans << DOS << "<li> " << EEPROMUserName(i).dos();
        }
      }
      ans << UTF8 << "</ul>";
}

void WebOptions::write_active_users(Ans &ans)
{
    uint32_t all_users = get_eeprom_users(US_Enabled|US_Paricipated|US_Done, US_Enabled|US_Paricipated);
    if (!all_users) return;

    uint32_t usrs_by_cnt[9] = {};
    for(int i=0; i<32; ++i) if (all_users & bit(i)) usrs_by_cnt[working_state.total_loaded_gift(i)] |= bit(i);

    ans << UTF8 << "Играют:<br><ul>";

    for(int t=0; t<=8; ++t)
    {
      uint32_t usr = usrs_by_cnt[t];
      for(int i=0; i<32; ++i)
      {
        if (usr & bit(i))
        {
          ans << UTF8 << "<li> ";
          if (working_state.enabled_users & bit(i)) ans << UTF8 << "<i class='fa fa-check-circle' style='font-size:24px'></i> ";
          ans << DOS << EEPROMUserName(i).dos() << " - ";
          if (!t) ans << UTF8 << "<b>НЕ ЗАГРУЖЕНО!</b>"; else
          {
            ans << UTF8 << t << " ";
            write_podarok(ans, t);
          }
        }
      }  
    }
    ans << UTF8 << "</ul>";
}

void  WebOptions::ActiveUsersList(Ans &ans)     // HTML block with list of still Active Users (as in done-user.html AJAX)
{
    write_active_users(ans);
}

void  WebOptions::DoneUsersList(Ans &ans)       // HTML block with list of Done Users (as in done-user.html AJAX)
{
    write_done_users(ans);
}

/*
  <option value="0">Антон</option>
  <option value="1">Vasya</option>
  <option value="2">Roman</option>
*/
void  WebOptions::HTMLOptionsUserList(Ans &ans) // HTML block with list of Users taken part in Game
{
  for(int i=0; i<32; ++i)
  {
    UserSetup usr;
    if (usr.load(i, NULL) && (usr.status & (US_Enabled | US_Paricipated)) == (US_Enabled | US_Paricipated))
    {
      ans << DOS << "<option value='" << i << "'>" << EEPROMUserName(i).dos() << "</option>";
    }
  }
}

//  ["Антон (1)", "Антон (2)", null, null, "Vasya", "Roman", null, null]
void  WebOptions::LoadedGiftDoors(Ans &ans)     // JSON list of titles on Doors
{
  ans << UTF8 << "[";
  for(int i=0; i<8; ++i)
  {
    working_state.write_user_name(ans, i, true);
    if (i < 7) ans << UTF8 << ",";
  }
  ans << UTF8 << "]";
}

void  WebOptions::MsgTitle(Ans &ans)            // Title of Message page
{
  ans << UTF8 << buf_msg_title.c_str();
}

void  WebOptions::Message(Ans &ans)             // Message in Message page
{
  ans << UTF8 << buf_message.c_str();
}

/*
  <img id="i1" class="img-box" onclick="select(1)" src="../bg/1.jpg">
  <img id="i2" class="img-box" onclick="select(2)" src="../bg/2.jpg">
  <img id="i3" class="img-box" onclick="select(3)" src="../bg/3.jpg">
  <img id="i4" class="img-box" onclick="select(4)" src="../bg/4.jpg">
  <img id="i5" class="img-box" onclick="select(5)" src="../bg/5.jpg">
  <img id="i6" class="img-box" onclick="select(6)" src="../bg/6.jpg">
*/  
void  WebOptions::HTMLBGImageList(Ans &ans)     // HTML block for background Images list
{
  extern BGImage bg_images;
  int state = 0;
  int idx;
  while( (idx = bg_images.scan_bg_images(state)) != -1)
  {
    ans << UTF8 << "<img id='i" << idx << "' class='img-box' onclick='select(" << idx << ")' src='../bg/" << idx << ".jpg'>";
  }
}

/*
  <option value="1">Задача 1</option>
  <option value="2">Задача 2</option>
  <option value="3">Задача 3</option>
  <option value="4">Задача 4</option>
*/
void  WebOptions::HTMLChallengeOptions(Ans &ans) // HTML block with challenges list
{
  for(const auto& idx: challenge_mgr().all_data())
  {
    if (current_user.priority < 200 && logged_in_user != idx.second) continue;
    ans << DOS << "<option value='" << idx.first << "'>" << challenge_mgr().get_dos_name(idx.first) << "</option>";
  }
}

uint16_t WebOptions::InterRoundTime()      // Time between Game rounds
{
    return global_setup.round_time;
}

/*
  <p><input type="checkbox" name="u1" checked="true">&nbsp;Пользователь 1</p>
  <p><input type="checkbox" name="u2" checked="true">&nbsp;Пользователь 2</p>
  <p><input type="checkbox" name="u3" checked="true">&nbsp;Пользователь 3</p>
*/
void  WebOptions::HTMLUserListWithSelection(Ans &ans) // HTML block with list of enabled users
{
  uint8_t name[33];
  for(int idx = 0; idx < 32; ++idx)
  {
    UserSetup usr;
    if (!usr.load(idx, name) || !(usr.status & US_Enabled)) continue;
    ans << UTF8 << "<p><input type='checkbox' name='u" << idx << "'";
    if (usr.status & US_Paricipated) ans << UTF8 << " checked='true'";
    ans << DOS << ">&nbsp;" << name << "</p>";
  }
}

/*
  <tr><th class='fgl-ptr' onclick='edit_user(this, 1, "Антон")'>Антон<td><div id='u1-0' class='fgl-box fgl-box-filled fgl-ptr' onclick='del_fg(this, 1,0, "Антон")'></div><td><div id='u1-1' class='fgl-box'></div><td><div id='u1-2' class='fgl-box'></div><td><div id='u1-3' class='fgl-box'></div></tr>
  <tr><th>Сергей<td><div id='u2-0' class='fgl-box fgl-box-filled'></div><td><div id='u2-1' class='fgl-box fgl-box-filled'></div><td><div id='u2-2' class='fgl-box  fgl-box-filled'></div><td><div id='u2-3' class='fgl-box fgl-box-filled'></div></tr>
*/
void  WebOptions::HTMLFGLibrary(Ans &ans)   // FingerPrints Library Viewer HTML block with users and FG items. onclick handlers depends on priority of user/current user and fill state of FG cell
{
  uint8_t name[33];
  uint8_t fp_index[32];

  Activity::FPAccess(NULL).access().readIndexTable(fp_index);

  for(int idx = 0, fp_idx=0; idx < 32; ++idx, fp_idx+=4)
  {
    UserSetup usr;
    if (!usr.load(idx, name)) continue;

    ans << UTF8 << "<tr><th";
    if (current_user.options & UO_CanEditFG) ans << DOS <<" class='fgl-ptr' onclick='edit_user(this, " << idx << ", \"" << name << "\")'";
    ans << DOS << ">" << name;
    for(int box=0, fpi = fp_idx; box<4; ++box, ++fpi)
    {
      ans << DOS << "<td><div id='u" << idx << "-" << box << "' class='fgl-box";
      if (fp_index[fpi>>3] & bit(fpi&7)) // Filled
      {
        ans << UTF8 << " fgl-box-filled";
        if (current_user.options & UO_CanEditFG) ans << DOS << " fgl-ptr' onclick='del_fg(this, " << idx << "," << box <<", \"" << name << "\")";
      }
      ans << UTF8 << "'></div>";
    }
    ans << UTF8 << "</tr>";
  }
}

void  WebOptions::FGEditorUser(Ans &ans)    // User for new FG ('нового пользователя' or 'пользователя XXXX')
{
  if (fg_editor_user == -1) ans << UTF8 << "нового пользователя";
  else ans << UTF8 << "пользователя " << DOS << EEPROMUserName(fg_editor_user).dos();
}

void WebOptions::set_fg_editor_user(int user_index)
{
  fg_editor_user = user_index;
  if (user_index == -1)
  {
    FGEditorFilledBoxes = 0;
    FGEditorFilledCircs = 0;
  }
  else
  {
    int x = FGEditorFilledBoxes = fge_get_filled_tpls(user_index);
    x = (x & (x<<2)) & 0x33;  // ....abcd -> ..ab..cd
    x = (x | (x<<1)) & 0x55;  // ..ab..cd -> .a.b.c.d
    FGEditorFilledCircs = x | (x<<1); // .a.b.c.d -> aabbccdd
  }
  FGEditorFillingBox = -1;  // Index of filling now box (or -1)
  FGEditorFillingCirc = -1; // Index of circle filled rigth now
}

void WebOptions::LogSystemSetup(Ans& ans) // Ext Log setup data
{
  log_send_setup(ans);
}

