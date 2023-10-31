#include "common.h"

#include "web_ajax_classes.h"


// G(gift_load, P2(I, door, I, user))
void AJAXDecoder_gift_load::run()
{

}

// G(unload_gift, P1(I, door))
void AJAXDecoder_unload_gift::run()
{

}

// G(open_door, P1(I, door))
void AJAXDecoder_open_door::run()
{

}

// G(done_user, P1(I, user))
void AJAXDecoder_done_user::run()
{

}

// G(set_interround_time, P1(I, value))
void AJAXDecoder_set_interround_time::run()
{

}

// G(add_challenge, P1(SD, value))
void AJAXDecoder_add_challenge::run()
{

}

// G(del_challenge, P1(I, index))
void AJAXDecoder_del_challenge::run()
{

}

// G(get_challenge, P1(I, index))
void AJAXDecoder_get_challenge::run()
{

}

// G(get_user_opts, P1(I, index))
void AJAXDecoder_get_user_opts::run()
{

}

// G(set_user_opt, P3(I, index, SU, name, SD, value))
void AJAXDecoder_set_user_opt::run()
{

}

// G(del_user, P1(I, index))
void AJAXDecoder_del_user::run()
{

}

// G(del_user_fg, P2(I, usr_index, I, fg_index))
void AJAXDecoder_del_user_fg::run()
{

}

// G(ping, P2(SU, id, I, cnt))
void AJAXDecoder_ping::run()
{

}

// G(update_users, P2(U, users, OV, load_gifts))
void AJAXDecoder_update_users::run()
{

}

// G(end_game, P0)
void AJAXDecoder_end_game::run()
{

}

// G(start_game, P1(U, users))
void AJAXDecoder_start_game::run()
{

}

// G(add_user, P0)
void AJAXDecoder_add_user::run()
{

}

// S(bg_add)
size_t AJAXDecoder_bg_add::consume_stream(uint8_t* data, size_t size, bool eof)
{
    return 0;
}

// G(bg_remove, P1(I, index))
void AJAXDecoder_bg_remove::run()
{

}

// P(update_challenge, P5(I, index, I, min_age, I, max_age, TD, text, TD, answer))
void AJAXDecoder_update_challenge::run()
{

}

// G(recalibrate_touch, P0)
void AJAXDecoder_recalibrate_touch::run()
{

}

// G(reset, P0)
void AJAXDecoder_reset::run()
{

}

// G(zap, P0)
void AJAXDecoder_zap::run()
{

}

// G(fg_view, P0)
void AJAXDecoder_fg_view::run()
{

}

// G(fg_edit, P1(OI, index))
void AJAXDecoder_fg_edit::run()
{

}

// G(fg_viewer_done, P0)
void AJAXDecoder_fg_viewer_done::run()
{

}

// G(fg_editor_done, P2(OSD, name, OI, age))
void AJAXDecoder_fg_editor_done::run()
{

}
