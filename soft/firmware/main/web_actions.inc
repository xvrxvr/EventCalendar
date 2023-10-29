#define WEB_ACTIONS_LIST \
    G(gift_load, P2(I, door, I, user)) \
    G(unload_gift, P1(I, door)) \
    G(open_door, P1(I, door)) \
    G(done_user, P1(I, user)) \
    G(set_interround_time, P1(I, value)) \
    G(add_challenge, P1(S, value)) \
    G(del_challenge, P1(I, index)) \
    G(get_challenge, P1(I, index)) \
    G(get_user_opts, P1(I, index)) \\
    G(set_user_opt, P3(I, index, S, name, S, value)) \
    G(del_user, P1(I, index)) \
    G(del_user_fg, P2(I, usr_index, I, fg_index)) \
    G(ping, P2(S, id, I, cnt)) \
    G(update_users, P2(U, users, OV, load_gifts)) \
    G(end_game, P0) \
    G(start_game, P1(OU, users)) \
    G(add_user, P0) \
    P(bg_add, P1(T, file)) \
    G(bg_remove, P1(I, index)) \
    P(update_challenge, P5(I, index, I, min_age, I, max_age, T, text, T, answer)) \
    G(recalibrate_touch, P0) \
    G(reset, P0) \
    G(zap, P0) \
    G(fg_view, P0) \
    G(fg_edit, P1(OI, index)) \
    G(fg_viewer_done, P0) \
    G(fg_editor_done, P2(OS, name, OI, age))


