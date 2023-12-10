#pragma once
#include "activity.h"

struct UserSetup;

namespace Interactive {

void entry();

const char* test_user_login(const UserSetup& current_user, int logged_in_user);

// Draw message (in UTF8) to LCD in centered Box
// If 'msg' is NULL draws last box + mesage on already locked LCD instance
void lcd_message(const char* msg, ...);

// Show message box with Valid/Invalid message (for fixed amount of time)
// Box not cleared on exit, execution suspended for Box duration
void msg_valid(bool is_valid);

// Draw 'Help' icon in top right corner
void draw_help_icon();

// test for Help icon touch
inline bool test_help_icon(const Action &a) {return a.touch.y < 32 && a.touch.x > RES_X-32;}

// Check if FP index belong to user that can help in opening door
bool check_open_door_fingerprint(const Action &);

} // namespace Interactive
