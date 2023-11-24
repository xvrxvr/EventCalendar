#include "common.h"
#include "web_files_access.h"

CDNDef decode_web_files_access_function(const char* key)
{
    extern const char add_game_user_html_start[] asm("_binary_add_game_user_html_start");
    extern const char add_game_user_html_end[] asm("_binary_add_game_user_html_end");
    extern const char admin_html_start[] asm("_binary_admin_html_start");
    extern const char admin_html_end[] asm("_binary_admin_html_end");
    extern const char edit_user_html_start[] asm("_binary_edit_user_html_start");
    extern const char edit_user_html_end[] asm("_binary_edit_user_html_end");
    extern const char fg_editor_html_start[] asm("_binary_fg_editor_html_start");
    extern const char fg_editor_html_end[] asm("_binary_fg_editor_html_end");
    extern const char fg_viewer_html_start[] asm("_binary_fg_viewer_html_start");
    extern const char fg_viewer_html_end[] asm("_binary_fg_viewer_html_end");
    extern const char jslib_js_start[] asm("_binary_jslib_js_start");
    extern const char jslib_js_end[] asm("_binary_jslib_js_end");
    extern const char load_gifts_html_start[] asm("_binary_load_gifts_html_start");
    extern const char load_gifts_html_end[] asm("_binary_load_gifts_html_end");
    extern const char message_html_start[] asm("_binary_message_html_start");
    extern const char message_html_end[] asm("_binary_message_html_end");
    extern const char open_door_html_start[] asm("_binary_open_door_html_start");
    extern const char open_door_html_end[] asm("_binary_open_door_html_end");
    extern const char setup_html_start[] asm("_binary_setup_html_start");
    extern const char setup_html_end[] asm("_binary_setup_html_end");
    extern const char set_bg_images_html_start[] asm("_binary_set_bg_images_html_start");
    extern const char set_bg_images_html_end[] asm("_binary_set_bg_images_html_end");
    extern const char set_challenge_html_start[] asm("_binary_set_challenge_html_start");
    extern const char set_challenge_html_end[] asm("_binary_set_challenge_html_end");
    extern const char start_game_html_start[] asm("_binary_start_game_html_start");
    extern const char start_game_html_end[] asm("_binary_start_game_html_end");
    extern const char styles_css_start[] asm("_binary_styles_css_start");
    extern const char styles_css_end[] asm("_binary_styles_css_end");
    extern const char text_lib_js_start[] asm("_binary_text_lib_js_start");
    extern const char text_lib_js_end[] asm("_binary_text_lib_js_end");

    switch(key[0])
    {
        case 'a':
            switch(key[1])
            {
                case 'd':
                    switch(key[2])
                    {
                        case 'd': if (strcmp(key+3, "_game_user.html") == 0) {return CDNDef{add_game_user_html_start, add_game_user_html_end};} else {return CDNDef{};}
                        case 'm': if (strcmp(key+3, "in.html") == 0) {return CDNDef{admin_html_start, admin_html_end};} else {return CDNDef{};}
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        case 'e': if (strcmp(key+1, "dit_user.html") == 0) {return CDNDef{edit_user_html_start, edit_user_html_end};} else {return CDNDef{};}
        case 'f': if (memcmp(key+1, "g_", 2) != 0) {return CDNDef{};}
                  switch(key[3])
                  {
                      case 'e': if (strcmp(key+4, "ditor.html") == 0) {return CDNDef{fg_editor_html_start, fg_editor_html_end};} else {return CDNDef{};}
                      case 'v': if (strcmp(key+4, "iewer.html") == 0) {return CDNDef{fg_viewer_html_start, fg_viewer_html_end};} else {return CDNDef{};}
                      default: return CDNDef{};
                  }
        case 'j': if (strcmp(key+1, "slib.js") == 0) {return CDNDef{jslib_js_start, jslib_js_end};} else {return CDNDef{};}
        case 'l': if (strcmp(key+1, "oad_gifts.html") == 0) {return CDNDef{load_gifts_html_start, load_gifts_html_end};} else {return CDNDef{};}
        case 'm': if (strcmp(key+1, "essage.html") == 0) {return CDNDef{message_html_start, message_html_end};} else {return CDNDef{};}
        case 'o': if (strcmp(key+1, "pen_door.html") == 0) {return CDNDef{open_door_html_start, open_door_html_end};} else {return CDNDef{};}
        case 's':
            switch(key[1])
            {
                case 'e':
                    switch(key[2])
                    {
                        case 't':
                            switch(key[3])
                            {
                                case '_':
                                    switch(key[4])
                                    {
                                        case 'b': if (strcmp(key+5, "g_images.html") == 0) {return CDNDef{set_bg_images_html_start, set_bg_images_html_end};} else {return CDNDef{};}
                                        case 'c': if (strcmp(key+5, "hallenge.html") == 0) {return CDNDef{set_challenge_html_start, set_challenge_html_end};} else {return CDNDef{};}
                                        default: return CDNDef{};
                                    }
                                case 'u': if (strcmp(key+4, "p.html") == 0) {return CDNDef{setup_html_start, setup_html_end};} else {return CDNDef{};}
                                default: return CDNDef{};
                            }
                        default: return CDNDef{};
                    }
                case 't':
                    switch(key[2])
                    {
                        case 'a': if (strcmp(key+3, "rt_game.html") == 0) {return CDNDef{start_game_html_start, start_game_html_end};} else {return CDNDef{};}
                        case 'y': if (strcmp(key+3, "les.css") == 0) {return CDNDef{styles_css_start, styles_css_end};} else {return CDNDef{};}
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        case 't': if (strcmp(key+1, "ext_lib.js") == 0) {return CDNDef{text_lib_js_start, text_lib_js_end};} else {return CDNDef{};}
        default: return CDNDef{};
    }
}
