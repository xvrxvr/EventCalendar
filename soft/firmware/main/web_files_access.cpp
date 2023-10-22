#include "web_files_access.h"
#include <string.h>
#include <stdint.h>

CDNDef decode_web_files_access_function(const char* key)
{
    extern const unsigned char web_add_game_user_html_start[] asm("_binary_web_add_game_user_html_start");
    extern const unsigned char web_add_game_user_html_end[] asm("_binary_web_add_game_user_html_end");
    extern const unsigned char web_admin_html_start[] asm("_binary_web_admin_html_start");
    extern const unsigned char web_admin_html_end[] asm("_binary_web_admin_html_end");
    extern const unsigned char web_edit_user_html_start[] asm("_binary_web_edit_user_html_start");
    extern const unsigned char web_edit_user_html_end[] asm("_binary_web_edit_user_html_end");
    extern const unsigned char web_fg_editor_html_start[] asm("_binary_web_fg_editor_html_start");
    extern const unsigned char web_fg_editor_html_end[] asm("_binary_web_fg_editor_html_end");
    extern const unsigned char web_fg_viewer_html_start[] asm("_binary_web_fg_viewer_html_start");
    extern const unsigned char web_fg_viewer_html_end[] asm("_binary_web_fg_viewer_html_end");
    extern const unsigned char web_jslib_js_start[] asm("_binary_web_jslib_js_start");
    extern const unsigned char web_jslib_js_end[] asm("_binary_web_jslib_js_end");
    extern const unsigned char web_load_gifts_html_start[] asm("_binary_web_load_gifts_html_start");
    extern const unsigned char web_load_gifts_html_end[] asm("_binary_web_load_gifts_html_end");
    extern const unsigned char web_message_html_start[] asm("_binary_web_message_html_start");
    extern const unsigned char web_message_html_end[] asm("_binary_web_message_html_end");
    extern const unsigned char web_open_door_html_start[] asm("_binary_web_open_door_html_start");
    extern const unsigned char web_open_door_html_end[] asm("_binary_web_open_door_html_end");
    extern const unsigned char web_setup_html_start[] asm("_binary_web_setup_html_start");
    extern const unsigned char web_setup_html_end[] asm("_binary_web_setup_html_end");
    extern const unsigned char web_set_bg_images_html_start[] asm("_binary_web_set_bg_images_html_start");
    extern const unsigned char web_set_bg_images_html_end[] asm("_binary_web_set_bg_images_html_end");
    extern const unsigned char web_set_challenge_html_start[] asm("_binary_web_set_challenge_html_start");
    extern const unsigned char web_set_challenge_html_end[] asm("_binary_web_set_challenge_html_end");
    extern const unsigned char web_start_game_html_start[] asm("_binary_web_start_game_html_start");
    extern const unsigned char web_start_game_html_end[] asm("_binary_web_start_game_html_end");
    extern const unsigned char web_styles_css_start[] asm("_binary_web_styles_css_start");
    extern const unsigned char web_styles_css_end[] asm("_binary_web_styles_css_end");

    switch(key[0])
    {
        case 'a':
            switch(key[1])
            {
                case 'd':
                    switch(key[2])
                    {
                        case 'd': return strcmp(key+3, "_game_user.html") == 0 ? CDNDef{web_add_game_user_html_start, web_add_game_user_html_end} : CDNDef{};
                        case 'm': return strcmp(key+3, "in.html") == 0 ? CDNDef{web_admin_html_start, web_admin_html_end} : CDNDef{};
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        case 'e': return strcmp(key+1, "dit_user.html") == 0 ? CDNDef{web_edit_user_html_start, web_edit_user_html_end} : CDNDef{};
        case 'f':
            switch(key[1])
            {
                case 'g':
                    switch(key[2])
                    {
                        case '_':
                            switch(key[3])
                            {
                                case 'e': return strcmp(key+4, "ditor.html") == 0 ? CDNDef{web_fg_editor_html_start, web_fg_editor_html_end} : CDNDef{};
                                case 'v': return strcmp(key+4, "iewer.html") == 0 ? CDNDef{web_fg_viewer_html_start, web_fg_viewer_html_end} : CDNDef{};
                                default: return CDNDef{};
                            }
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        case 'j': return strcmp(key+1, "slib.js") == 0 ? CDNDef{web_jslib_js_start, web_jslib_js_end} : CDNDef{};
        case 'l': return strcmp(key+1, "oad_gifts.html") == 0 ? CDNDef{web_load_gifts_html_start, web_load_gifts_html_end} : CDNDef{};
        case 'm': return strcmp(key+1, "essage.html") == 0 ? CDNDef{web_message_html_start, web_message_html_end} : CDNDef{};
        case 'o': return strcmp(key+1, "pen_door.html") == 0 ? CDNDef{web_open_door_html_start, web_open_door_html_end} : CDNDef{};
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
                                        case 'b': return strcmp(key+5, "g_images.html") == 0 ? CDNDef{web_set_bg_images_html_start, web_set_bg_images_html_end} : CDNDef{};
                                        case 'c': return strcmp(key+5, "hallenge.html") == 0 ? CDNDef{web_set_challenge_html_start, web_set_challenge_html_end} : CDNDef{};
                                        default: return CDNDef{};
                                    }
                                case 'u': return strcmp(key+4, "p.html") == 0 ? CDNDef{web_setup_html_start, web_setup_html_end} : CDNDef{};
                                default: return CDNDef{};
                            }
                        default: return CDNDef{};
                    }
                case 't':
                    switch(key[2])
                    {
                        case 'a': return strcmp(key+3, "rt_game.html") == 0 ? CDNDef{web_start_game_html_start, web_start_game_html_end} : CDNDef{};
                        case 'y': return strcmp(key+3, "les.css") == 0 ? CDNDef{web_styles_css_start, web_styles_css_end} : CDNDef{};
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        default: return CDNDef{};
    }
}
