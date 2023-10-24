#include "web_files_access.h"
#include <string.h>
#include <stdint.h>

CDNDef decode_web_files_access_function(const char* key)
{
    extern const char web_add_game_user_html_start[] asm("_binary_web_add_game_user_html_start");
    extern const char web_add_game_user_html_end[] asm("_binary_web_add_game_user_html_end");
    extern const char web_admin_html_start[] asm("_binary_web_admin_html_start");
    extern const char web_admin_html_end[] asm("_binary_web_admin_html_end");
    extern const char web_edit_user_html_start[] asm("_binary_web_edit_user_html_start");
    extern const char web_edit_user_html_end[] asm("_binary_web_edit_user_html_end");
    extern const char web_fg_editor_html_start[] asm("_binary_web_fg_editor_html_start");
    extern const char web_fg_editor_html_end[] asm("_binary_web_fg_editor_html_end");
    extern const char web_fg_viewer_html_start[] asm("_binary_web_fg_viewer_html_start");
    extern const char web_fg_viewer_html_end[] asm("_binary_web_fg_viewer_html_end");
    extern const char web_jslib_js_start[] asm("_binary_web_jslib_js_start");
    extern const char web_jslib_js_end[] asm("_binary_web_jslib_js_end");
    extern const char web_load_gifts_html_start[] asm("_binary_web_load_gifts_html_start");
    extern const char web_load_gifts_html_end[] asm("_binary_web_load_gifts_html_end");
    extern const char web_message_html_start[] asm("_binary_web_message_html_start");
    extern const char web_message_html_end[] asm("_binary_web_message_html_end");
    extern const char web_open_door_html_start[] asm("_binary_web_open_door_html_start");
    extern const char web_open_door_html_end[] asm("_binary_web_open_door_html_end");
    extern const char web_setup_html_start[] asm("_binary_web_setup_html_start");
    extern const char web_setup_html_end[] asm("_binary_web_setup_html_end");
    extern const char web_set_bg_images_html_start[] asm("_binary_web_set_bg_images_html_start");
    extern const char web_set_bg_images_html_end[] asm("_binary_web_set_bg_images_html_end");
    extern const char web_set_challenge_html_start[] asm("_binary_web_set_challenge_html_start");
    extern const char web_set_challenge_html_end[] asm("_binary_web_set_challenge_html_end");
    extern const char web_start_game_html_start[] asm("_binary_web_start_game_html_start");
    extern const char web_start_game_html_end[] asm("_binary_web_start_game_html_end");
    extern const char web_styles_css_start[] asm("_binary_web_styles_css_start");
    extern const char web_styles_css_end[] asm("_binary_web_styles_css_end");

    switch(key[0])
    {
        case 'a':
            switch(key[1])
            {
                case 'd':
                    switch(key[2])
                    {
                        case 'd': if (strcmp(key+3, "_game_user.html") == 0) {return CDNDef{web_add_game_user_html_start, web_add_game_user_html_end};} else {return CDNDef{};}
                        case 'm': if (strcmp(key+3, "in.html") == 0) {return CDNDef{web_admin_html_start, web_admin_html_end};} else {return CDNDef{};}
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        case 'e': if (strcmp(key+1, "dit_user.html") == 0) {return CDNDef{web_edit_user_html_start, web_edit_user_html_end};} else {return CDNDef{};}
        case 'f': if (memcmp(key+1, "g_", 2) != 0) {return CDNDef{};}
                  switch(key[3])
                  {
                      case 'e': if (strcmp(key+4, "ditor.html") == 0) {return CDNDef{web_fg_editor_html_start, web_fg_editor_html_end};} else {return CDNDef{};}
                      case 'v': if (strcmp(key+4, "iewer.html") == 0) {return CDNDef{web_fg_viewer_html_start, web_fg_viewer_html_end};} else {return CDNDef{};}
                      default: return CDNDef{};
                  }
        case 'j': if (strcmp(key+1, "slib.js") == 0) {return CDNDef{web_jslib_js_start, web_jslib_js_end};} else {return CDNDef{};}
        case 'l': if (strcmp(key+1, "oad_gifts.html") == 0) {return CDNDef{web_load_gifts_html_start, web_load_gifts_html_end};} else {return CDNDef{};}
        case 'm': if (strcmp(key+1, "essage.html") == 0) {return CDNDef{web_message_html_start, web_message_html_end};} else {return CDNDef{};}
        case 'o': if (strcmp(key+1, "pen_door.html") == 0) {return CDNDef{web_open_door_html_start, web_open_door_html_end};} else {return CDNDef{};}
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
                                        case 'b': if (strcmp(key+5, "g_images.html") == 0) {return CDNDef{web_set_bg_images_html_start, web_set_bg_images_html_end};} else {return CDNDef{};}
                                        case 'c': if (strcmp(key+5, "hallenge.html") == 0) {return CDNDef{web_set_challenge_html_start, web_set_challenge_html_end};} else {return CDNDef{};}
                                        default: return CDNDef{};
                                    }
                                case 'u': if (strcmp(key+4, "p.html") == 0) {return CDNDef{web_setup_html_start, web_setup_html_end};} else {return CDNDef{};}
                                default: return CDNDef{};
                            }
                        default: return CDNDef{};
                    }
                case 't':
                    switch(key[2])
                    {
                        case 'a': if (strcmp(key+3, "rt_game.html") == 0) {return CDNDef{web_start_game_html_start, web_start_game_html_end};} else {return CDNDef{};}
                        case 'y': if (strcmp(key+3, "les.css") == 0) {return CDNDef{web_styles_css_start, web_styles_css_end};} else {return CDNDef{};}
                        default: return CDNDef{};
                    }
                default: return CDNDef{};
            }
        default: return CDNDef{};
    }
}
