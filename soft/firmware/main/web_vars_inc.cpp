#include <string.h>
#include "web_vars.h"

void WebOptions::decode_inline(const char* key, Ans& ans)
{
    switch(key[0])
    {
        case 'A': if (strcmp(key+1, "ctiveUsersList") == 0) {ActiveUsersList(ans); return;} else {err_novar(key); return;}
        case 'C': if (memcmp(key+1, "ur", 2) != 0) {err_novar(key); return;}
                  switch(key[3])
                  {
                      case 'U': if (strcmp(key+4, "serPrio") == 0) {ans.write_int(CurUserPrio); return;} else {err_novar(key); return;}
                      case 'r': if (strcmp(key+4, "entUser") == 0) {CurrentUser(ans); return;} else {err_novar(key); return;}
                      default: err_novar(key); return;
                  }
        case 'D': if (strcmp(key+1, "oneUsersList") == 0) {DoneUsersList(ans); return;} else {err_novar(key); return;}
        case 'F': if (memcmp(key+1, "GEditor", 7) != 0) {err_novar(key); return;}
                  switch(key[8])
                  {
                      case 'F': if (memcmp(key+9, "ill", 3) != 0) {err_novar(key); return;}
                                switch(key[12])
                                {
                                    case 'e':
                                        switch(key[13])
                                        {
                                            case 'd':
                                                switch(key[14])
                                                {
                                                    case 'B': if (strcmp(key+15, "oxes") == 0) {ans.write_int(FGEditorFilledBoxes); return;} else {err_novar(key); return;}
                                                    case 'C': if (strcmp(key+15, "ircs") == 0) {ans.write_int(FGEditorFilledCircs); return;} else {err_novar(key); return;}
                                                    default: err_novar(key); return;
                                                }
                                            default: err_novar(key); return;
                                        }
                                    case 'i': if (memcmp(key+13, "ng", 2) != 0) {err_novar(key); return;}
                                              switch(key[15])
                                              {
                                                  case 'B': if (strcmp(key+16, "ox") == 0) {ans.write_int(FGEditorFillingBox); return;} else {err_novar(key); return;}
                                                  case 'C': if (strcmp(key+16, "irc") == 0) {ans.write_int(FGEditorFillingCirc); return;} else {err_novar(key); return;}
                                                  default: err_novar(key); return;
                                              }
                                    default: err_novar(key); return;
                                }
                      case 'U': if (strcmp(key+9, "ser") == 0) {FGEditorUser(ans); return;} else {err_novar(key); return;}
                      default: err_novar(key); return;
                  }
        case 'H': if (memcmp(key+1, "TML", 3) != 0) {err_novar(key); return;}
                  switch(key[4])
                  {
                      case 'B': if (strcmp(key+5, "GImageList") == 0) {HTMLBGImageList(ans); return;} else {err_novar(key); return;}
                      case 'C': if (strcmp(key+5, "hallengeOptions") == 0) {HTMLChallengeOptions(ans); return;} else {err_novar(key); return;}
                      case 'F': if (strcmp(key+5, "GLibrary") == 0) {HTMLFGLibrary(ans); return;} else {err_novar(key); return;}
                      case 'O': if (strcmp(key+5, "ptionsUserList") == 0) {HTMLOptionsUserList(ans); return;} else {err_novar(key); return;}
                      case 'U': if (memcmp(key+5, "serList", 7) != 0) {err_novar(key); return;}
                                switch(key[12])
                                {
                                    case 0: HTMLUserList(ans); return;
                                    case 'W': if (strcmp(key+13, "ithSelection") == 0) {HTMLUserListWithSelection(ans); return;} else {err_novar(key); return;}
                                    default: err_novar(key); return;
                                }
                      default: err_novar(key); return;
                  }
        case 'I': if (strcmp(key+1, "nterRoundTime") == 0) {ans.write_int(InterRoundTime()); return;} else {err_novar(key); return;}
        case 'L': if (strcmp(key+1, "oadedGiftDoors") == 0) {LoadedGiftDoors(ans); return;} else {err_novar(key); return;}
        case 'M':
            switch(key[1])
            {
                case 'a': if (strcmp(key+2, "inStatus") == 0) {MainStatus(ans); return;} else {err_novar(key); return;}
                case 'e': if (strcmp(key+2, "ssage") == 0) {Message(ans); return;} else {err_novar(key); return;}
                case 's': if (strcmp(key+2, "gTitle") == 0) {MsgTitle(ans); return;} else {err_novar(key); return;}
                default: err_novar(key); return;
            }
        case 'T': if (strcmp(key+1, "imeToDoorsEnable") == 0) {ans.write_int(TimeToDoorsEnable()); return;} else {err_novar(key); return;}
        case 'U': if (strcmp(key+1, "serRights") == 0) {ans.write_int(UserRights); return;} else {err_novar(key); return;}
        default: err_novar(key); return;
    }
}

uint32_t WebOptions::get_condition(const char* key, Ans& ans)
{
    switch(key[0])
    {
        case 'A': if (strcmp(key+1, "ctiveUsersList") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
        case 'C': if (memcmp(key+1, "ur", 2) != 0) {err_novar(key); return 0;}
                  switch(key[3])
                  {
                      case 'U': if (strcmp(key+4, "serPrio") == 0) {return CurUserPrio;} else {err_novar(key); return 0;}
                      case 'r': if (strcmp(key+4, "entUser") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                      default: err_novar(key); return 0;
                  }
        case 'D': if (strcmp(key+1, "oneUsersList") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
        case 'F': if (memcmp(key+1, "GEditor", 7) != 0) {err_novar(key); return 0;}
                  switch(key[8])
                  {
                      case 'F': if (memcmp(key+9, "ill", 3) != 0) {err_novar(key); return 0;}
                                switch(key[12])
                                {
                                    case 'e':
                                        switch(key[13])
                                        {
                                            case 'd':
                                                switch(key[14])
                                                {
                                                    case 'B': if (strcmp(key+15, "oxes") == 0) {return FGEditorFilledBoxes;} else {err_novar(key); return 0;}
                                                    case 'C': if (strcmp(key+15, "ircs") == 0) {return FGEditorFilledCircs;} else {err_novar(key); return 0;}
                                                    default: err_novar(key); return 0;
                                                }
                                            default: err_novar(key); return 0;
                                        }
                                    case 'i': if (memcmp(key+13, "ng", 2) != 0) {err_novar(key); return 0;}
                                              switch(key[15])
                                              {
                                                  case 'B': if (strcmp(key+16, "ox") == 0) {return FGEditorFillingBox;} else {err_novar(key); return 0;}
                                                  case 'C': if (strcmp(key+16, "irc") == 0) {return FGEditorFillingCirc;} else {err_novar(key); return 0;}
                                                  default: err_novar(key); return 0;
                                              }
                                    default: err_novar(key); return 0;
                                }
                      case 'U': if (strcmp(key+9, "ser") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                      default: err_novar(key); return 0;
                  }
        case 'H': if (memcmp(key+1, "TML", 3) != 0) {err_novar(key); return 0;}
                  switch(key[4])
                  {
                      case 'B': if (strcmp(key+5, "GImageList") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                      case 'C': if (strcmp(key+5, "hallengeOptions") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                      case 'F': if (strcmp(key+5, "GLibrary") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                      case 'O': if (strcmp(key+5, "ptionsUserList") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                      case 'U': if (memcmp(key+5, "serList", 7) != 0) {err_novar(key); return 0;}
                                switch(key[12])
                                {
                                    case 0: err_type_wrong(key); return 0;
                                    case 'W': if (strcmp(key+13, "ithSelection") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                                    default: err_novar(key); return 0;
                                }
                      default: err_novar(key); return 0;
                  }
        case 'I': if (strcmp(key+1, "nterRoundTime") == 0) {return InterRoundTime();} else {err_novar(key); return 0;}
        case 'L': if (strcmp(key+1, "oadedGiftDoors") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
        case 'M':
            switch(key[1])
            {
                case 'a': if (strcmp(key+2, "inStatus") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                case 'e': if (strcmp(key+2, "ssage") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                case 's': if (strcmp(key+2, "gTitle") == 0) {err_type_wrong(key); return 0;} else {err_novar(key); return 0;}
                default: err_novar(key); return 0;
            }
        case 'T': if (strcmp(key+1, "imeToDoorsEnable") == 0) {return TimeToDoorsEnable();} else {err_novar(key); return 0;}
        case 'U': if (strcmp(key+1, "serRights") == 0) {return UserRights;} else {err_novar(key); return 0;}
        default: err_novar(key); return 0;
    }
}

