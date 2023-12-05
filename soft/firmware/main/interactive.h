#pragma once

struct UserSetup;

namespace Interactive {

void entry();

const char* test_user_login(const UserSetup& current_user, int logged_in_user);

} // namespace Interactive
