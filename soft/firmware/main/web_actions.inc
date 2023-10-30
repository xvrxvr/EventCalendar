#ifndef P
    #define P G
#endif 

#ifndef VP0
    #define VP0
    #define CP0
#endif

#ifndef VP1
    #define VP1(Type, Id) VARG(Type, Id)
    #define CP1(Type, Id) CARG(Type, Id)
#endif

#ifndef VP2
    #define VP2(Type, Id, ...) VARG(Type, Id) VP1(__VA_ARGS__) 
    #define CP2(Type, Id, ...) CARG(Type, Id) CP1(__VA_ARGS__)
#endif

#ifndef VP3
    #define VP3(Type, Id, ...) VARG(Type, Id) VP2(__VA_ARGS__)
    #define CP3(Type, Id, ...) CARG(Type, Id) CP2(__VA_ARGS__)
#endif

#ifndef VP4
    #define VP4(Type, Id, ...) VARG(Type, Id) VP3(__VA_ARGS__)
    #define CP4(Type, Id, ...) CARG(Type, Id) CP3(__VA_ARGS__)
#endif

#ifndef VP5
    #define VP5(Type, Id, ...) VARG(Type, Id) VP4(__VA_ARGS__)
    #define CP5(Type, Id, ...) CARG(Type, Id) CP4(__VA_ARGS__)
#endif

G(gift_load, P2(I, door, I, user))
G(unload_gift, P1(I, door))
G(open_door, P1(I, door))
G(done_user, P1(I, user))
G(set_interround_time, P1(I, value))
G(add_challenge, P1(SD, value))
G(del_challenge, P1(I, index))
G(get_challenge, P1(I, index))
G(get_user_opts, P1(I, index))
G(set_user_opt, P3(I, index, SU, name, SD, value))
G(del_user, P1(I, index))
G(del_user_fg, P2(I, usr_index, I, fg_index))
G(ping, P2(SU, id, I, cnt))
G(update_users, P2(U, users, OV, load_gifts))
G(end_game, P0)
G(start_game, P1(U, users))
G(add_user, P0)
P(bg_add, P1(TU, file))
G(bg_remove, P1(I, index))
P(update_challenge, P5(I, index, I, min_age, I, max_age, TD, text, TD, answer))
G(recalibrate_touch, P0)
G(reset, P0)
G(zap, P0)
G(fg_view, P0)
G(fg_edit, P1(OI, index))
G(fg_viewer_done, P0)
G(fg_editor_done, P2(OSD, name, OI, age))

#undef G
#undef P

#undef VP0
#undef VP1
#undef VP2
#undef VP3
#undef VP4
#undef VP5

#undef CP0
#undef CP1
#undef CP2
#undef CP3
#undef CP4
#undef CP5

#ifdef CARG
    #undef CARG
    #undef VARG
#endif

/*
Macros:
G(<url>,<args>) - GET request to <url> wit parameters <args> (one of P<n> macro)
P(<url>,<args>) - POST request to <url> wit parameters <args> (one of P<n> macro)

P<n> - Params pack. <n> - number of arguments. Parameters - { <type>, <mame> }
Types is:
    I   -integer
    OI  - Optional integer
    SU  - UTF8 string
    SD  - DOS string (recode UTF8 -> DOS)
    OSD - Optional DOS string (recode UTF8 -> DOS)
    TU - Large Text in UTF8
    TD - Large Text in DOS (recode)
    U  - Series of u<n> parameters. All of them packed in one integer
    OV - Flag (Optional void). bool - set to true if parameter exists

User P<n> - P0, P1, P2, P3, P5
*/
