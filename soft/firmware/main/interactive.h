#pragma once

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

} // namespace Interactive
