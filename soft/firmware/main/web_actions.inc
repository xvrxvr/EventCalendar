#ifndef P
    #define P G
#endif

#ifndef GZ
    #define GZ P
#endif    

#ifndef S
    #define S(id) P(id, P0)
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
    #define VP2(Type, Id, p1, p2) VARG(Type, Id) VP1(p1, p2) 
    #define CP2(Type, Id, p1, p2) CARG(Type, Id) CP1(p1, p2)
#endif

#ifndef VP3
    #define VP3(Type, Id, p1, p2, p3, p4) VARG(Type, Id) VP2(p1, p2, p3, p4)
    #define CP3(Type, Id, p1, p2, p3, p4) CARG(Type, Id) CP2(p1, p2, p3, p4)
#endif

#ifndef VP4
    #define VP4(Type, Id, p1, p2, p3, p4, p5, p6) VARG(Type, Id) VP3(p1, p2, p3, p4, p5, p6)
    #define CP4(Type, Id, p1, p2, p3, p4, p5, p6) CARG(Type, Id) CP3(p1, p2, p3, p4, p5, p6)
#endif

#ifndef VP5
    #define VP5(Type, Id, p1, p2, p3, p4, p5, p6, p7, p8) VARG(Type, Id) VP4(p1, p2, p3, p4, p5, p6, p7, p8)
    #define CP5(Type, Id, p1, p2, p3, p4, p5, p6, p7, p8) CARG(Type, Id) CP4(p1, p2, p3, p4, p5, p6, p7, p8)
#endif

G(gift_load, P2(I, door, I, user))
G(unload_gift, P1(I, door))
G(open_door, P1(I, door))
G(done_user, P1(I, user))
G(set_interround_time, P1(I, value))
G(del_challenge, P1(I, index))
G(get_challenge, P1(I, index))
G(get_user_opts, P1(I, index))
G(set_user_opt, P3(I, index, SU, name, SD, value))
G(del_user, P1(I, index))
G(del_user_fg, P2(I, usr_index, I, fg_index))
G(ping, P2(SU, id, I, cnt))
G(update_users, P2(U, users, OV, load_gifts))
G(end_game, P0)
G(start_game, P2(U, users, OSU, start_time))
G(add_user, P0)
S(bg_add)
G(bg_remove, P1(I, index))
S(update_challenge)
G(recalibrate_touch, P0)
G(reset, P0)
G(zap, P0)
G(fg_view, P0)
G(fg_edit, P1(OI, index))
G(fg_viewer_done, P0)
G(fg_editor_done, P2(OSD, name, OI, age))

S(bkp_restore)
GZ(bkp_save, P0)

S(fw_update)

G(test_challenge, P1(I, id))

#undef G
#undef P
#undef S
#undef GZ

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
G(<url>,<args>) - GET request to <url> with parameters <args> (one of P<n> macro)
P(<url>,<args>) - POST request to <url> with parameters <args> (one of P<n> macro)
S(<url>) - POST request with 1 file. Should be accepted in streaming manner

P<n> - Params pack. <n> - number of arguments. Parameters - { <type>, <name> }
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

Used P<n> - P0, P1, P2, P3, P5
*/
