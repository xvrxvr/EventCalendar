#pragma once

#include "web_gadgets.h"
#include "prnbuf.h"

struct WebOptions {
    uint8_t GameStarted();          // 1 if game started, 2 if not
    void CurrentUser(Ans&);         // Name of currently logged on user
    void MainStatus(Ans&);          // HTML block with current status of system
    void HTMLUserList(Ans&);        // HTML block with all Users available to edit by current
/*
   <option value="1">Пользователь 1</option>
   <option value="2">Пользователь 2</option>
   <option value="3">Пользователь 3</option>
   <option value="4">Пользователь 4</option>
*/

    uint16_t UserRights();          // Rights of currently logged-in user
    uint8_t  CurUserPrio();         // Priority value of Current User
    uint8_t  TimeToDoorsEnable();   // Time to reenargizing Door system

    void ActiveUsersList(Ans&);     // HTML block with list of still Active Users (as in done-user.html AJAX)
    void DoneUsersList(Ans&);       // HTML block with list of Done Users (as in done-user.html AJAX)
    void HTMLOptionsUserList(Ans&); // HTML block with list of Users taken part in Game
/*
  <option value="0">Антон</option>
  <option value="1">Vasya</option>
  <option value="2">Roman</option>
*/

    void LoadedGiftDoors(Ans&);     // JSON list of titles on Doors
//  ['Антон (1)', 'Антон (2)', null, null, 'Vasya', 'Roman', null, null]

    void MsgTitle(Ans&);            // Title of Message page
    void Message(Ans&);             // Message in Message page
    void HTMLBGImageList(Ans&);     // HTML block for background Images list
/*
  <img id="i1" class="img-box" onclick="select(1)" src="../bg/1.png">
  <img id="i2" class="img-box" onclick="select(2)" src="../bg/2.png">
  <img id="i3" class="img-box" onclick="select(3)" src="../bg/3.png">
  <img id="i4" class="img-box" onclick="select(4)" src="../bg/4.png">
  <img id="i5" class="img-box" onclick="select(5)" src="../bg/5.png">
  <img id="i6" class="img-box" onclick="select(6)" src="../bg/6.png">
*/
    void HTMLChallengeOptions(Ans&); // HTML block with challenges list
/*
  <option value="1">Задача 1</option>
  <option value="2">Задача 2</option>
  <option value="3">Задача 3</option>
  <option value="4">Задача 4</option>
*/
    uint16_t InterRoundTime();      // Time between Game rounds
    void HTMLUserListWithSelection(Ans&); // HTML block with list of enabled users
/*
  <p><input type="checkbox" name="u1" checked="true">&nbsp;Пользователь 1</p>
  <p><input type="checkbox" name="u2" checked="true">&nbsp;Пользователь 2</p>
  <p><input type="checkbox" name="u3" checked="true">&nbsp;Пользователь 3</p>
*/
    void HTMLFGLibrary(Ans&);   // FingerPrints Library Viewer HTML block with users and FG items. onclick handlers depends on priority of user/current user and fill state of FG cell
/*
  <tr><th class="fgl-ptr" onclick="edit_user(this, 1, 'Антон')">Антон<td><div id="u1-0" class="fgl-box fgl-box-filled fgl-ptr" onclick="del_fg(this, 1,0, 'Антон')"></div><td><div id="u1-1" class="fgl-box"></div><td><div id="u1-2" class="fgl-box"></div><td><div id="u1-3" class="fgl-box"></div></tr>
  <tr><th>Сергей<td><div id="u2-0" class="fgl-box fgl-box-filled"></div><td><div id="u2-1" class="fgl-box fgl-box-filled"></div><td><div id="u2-2" class="fgl-box  fgl-box-filled"></div><td><div id="u2-3" class="fgl-box fgl-box-filled"></div></tr>
*/
    void FGEditorUser(Ans&);    // User for new FG ('нового пользователя' or 'пользователя XXXX')
    uint8_t FGEditorFilledBoxes; // Bitscale of filled boxes for current user in FG editor
    int8_t FGEditorFillingBox;  // Index of filling now box (or -1)
    uint8_t FGEditorFilledCircs; // Bitscale of filled circles  (index is <block-number>*2 + <circle-number>)
    int8_t FGEditorFillingCirc; // Index of circle filled rigth now

    void decode_inline(const char* ptr, Ans&);
    uint32_t get_condition(const char* ptr, Ans&);

    // Vars MsgTitle and Message, both in UTF8
    void set_title_and_message(const char* title, const char* message)
    {
      buf_msg_title.strcpy(title);
      buf_message.strcpy(message);
    }

    void set_fg_editor_user(int user_index=-1);

    void write_done_users(Ans&);
    void write_active_users(Ans&);
    
private:
    void err_novar(const char*);
    void err_type_wrong(const char*);

    Prn buf_msg_title, buf_message;
    int fg_editor_user = -1;
};

extern WebOptions web_options;
