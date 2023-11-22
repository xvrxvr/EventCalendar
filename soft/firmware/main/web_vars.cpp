#include "common.h"

#include "web_vars.h"
#include "setup_data.h"
#include "hadrware.h"

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

void WebOptions::CurrentUser(Ans &ans)         // Name of currently logged on user
{
    ans << DOS << (char*)current_user_name;
}

void WebOptions::MainStatus(Ans &ans)          // HTML block with current status of system
{

}

/*
   <option value="1">Пользователь 1</option>
   <option value="2">Пользователь 2</option>
   <option value="3">Пользователь 3</option>
   <option value="4">Пользователь 4</option>
*/
void WebOptions::HTMLUserList(Ans &ans)        // HTML block with all Users available to edit by current
{

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

void  WebOptions::ActiveUsersList(Ans &ans)     // HTML block with list of still Active Users (as in done-user.html AJAX)
{

}

void  WebOptions::DoneUsersList(Ans &ans)       // HTML block with list of Done Users (as in done-user.html AJAX)
{

}

/*
  <option value="0">Антон</option>
  <option value="1">Vasya</option>
  <option value="2">Roman</option>
*/
void  WebOptions::HTMLOptionsUserList(Ans &ans) // HTML block with list of Users taken part in Game
{

}


//  ["Антон (1)", "Антон (2)", null, null, "Vasya", "Roman", null, null]
void  WebOptions::LoadedGiftDoors(Ans &ans)     // JSON list of titles on Doors
{

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
  <img id="i1" class="img-box" onclick="select(1)" src="../bg/1.png">
  <img id="i2" class="img-box" onclick="select(2)" src="../bg/2.png">
  <img id="i3" class="img-box" onclick="select(3)" src="../bg/3.png">
  <img id="i4" class="img-box" onclick="select(4)" src="../bg/4.png">
  <img id="i5" class="img-box" onclick="select(5)" src="../bg/5.png">
  <img id="i6" class="img-box" onclick="select(6)" src="../bg/6.png">
*/  
void  WebOptions::HTMLBGImageList(Ans &ans)     // HTML block for background Images list
{

}

/*
  <option value="1">Задача 1</option>
  <option value="2">Задача 2</option>
  <option value="3">Задача 3</option>
  <option value="4">Задача 4</option>
*/
void  WebOptions::HTMLChallengeOptions(Ans &ans) // HTML block with challenges list
{

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

}

/*
  <tr><th class="fgl-ptr" onclick="edit_user(this, 1, 'Антон')">Антон<td><div id="u1-0" class="fgl-box fgl-box-filled fgl-ptr" onclick="del_fg(this, 1,0, 'Антон')"></div><td><div id="u1-1" class="fgl-box"></div><td><div id="u1-2" class="fgl-box"></div><td><div id="u1-3" class="fgl-box"></div></tr>
  <tr><th>Сергей<td><div id="u2-0" class="fgl-box fgl-box-filled"></div><td><div id="u2-1" class="fgl-box fgl-box-filled"></div><td><div id="u2-2" class="fgl-box  fgl-box-filled"></div><td><div id="u2-3" class="fgl-box fgl-box-filled"></div></tr>
*/
void  WebOptions::HTMLFGLibrary(Ans &ans)   // FingerPrints Library Viewer HTML block with users and FG items. onclick handlers depends on priority of user/current user and fill state of FG cell
{

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
