#pragma once

#include <string.h>

// This module contains various setup data thet holds in EEPROM and RTC RAM

static constexpr int max_users = 32;

// Options for User
enum UserOptions {
    UO_Admin                = 0x0001,   // This User is Admin

    UO_CanManageBG          = 0x0002,   // This Admin can add/delete/disable background images
    UO_CanSyncTime          = 0x0004,   // This Admin can forcibly sync time (or enter it manually)
    UO_CanSetup             = 0x0008,   // This Admin can change setup for timings (time sync intervals) and visibility (time, temperature), touch calibration, full system reset

    UO_CanStartRound        = 0x0010,   // This Admin can start/stop Play Round
    UO_CanOpenDoors         = 0x0020,   // This Admin can forcibly open any door
    UO_CanLoadGifts         = 0x0040,   // This Admin can load (and unload) gifts
    UO_CanSetupRoundTime    = 0x0080,   // This Admin can change Round time

    UO_CanEditUser          = 0x0100,   // This Admin can edit another User definition
    UO_CanAddRemoveUser     = 0x0200,   // This Admin can add or remove Users
    UO_CanAddRemoveAdmin    = 0x0400,   // This Admin can add or remove another Admint
    UO_CanSelectRound       = 0x0800,   // This Admin can select/deselect Users for upcoming Play Round
    UO_CanChangeUType       = 0x1000,   // This Admin can change User <-> Admin types
    UO_CanDisableUser       = 0x2000,   // This Admin can temporary disable/enable user
    UO_CanHelpUser          = 0x4000    // This Admin can help another User to bypass a challenge (by logging in and pressing button on WEB interface)
};

// Status of User
enum UserStatus {
    US_Enabled      = 0x01,     // User is Enabled (via UO_CanDisableUser option)
    US_Paricipated  = 0x02,     // User participated in Play Round
    US_Done         = 0x04,     // No more gifts expected for this User
};

// Setup data for User (placed in EEPROM, size is 16). Setup from all FF means unused entry
struct UserSetup {
    uint16_t    options; // Bitset of UserOptions
    uint8_t     status;  // Bitset of UserStatus
    uint8_t     priority; // User/Admin priority. Admin can edit/create only Users/Admins with priority <= self.priority
    uint8_t     age;    // User Age (for customizing challenges)
    uint8_t     reserved[3+8];
    
    void load(int usr_index, uint8_t name[32]);
    void save(int usr_index, uint8_t name[32]) const;

    bool empty() const {return options == 0xFFFF && status == 0xFF && priority == 0xFF && age == 0xFF;}
    void clear() {memset(this, 0xFF, sizeof(*this));}
};
static_assert(sizeof(UserSetup) == 16, "UserSetup size wrong");

// Touchscreen setup (24 bytes)
struct TouchSetup {
    int32_t A, B, C, D, E, F; // Fixed point values. X / (1<<31). -1 (FFF...F) for all values mean 'uncolibrated'
    
    void calibrate();

    bool empty() const {return A==-1 && B==-1 && C==-1 && D==-1 && E==-1 && F==-1;}
    void clear() {memset(this, 0xFF, sizeof(*this));}
    
    int32_t x(int32_t x, int32_t y);
    int32_t y(int32_t x, int32_t y);

    void sync() const; // Save me to EEPROM
};
static_assert(sizeof(TouchSetup) < 32, "TouchSetup structure overflow");

// Global setup option (on bit flags)
enum GlobalOptions {
    GO_HideRTC      = 0x0001,   // Do not show RTC time
    GO_HiteTemp     = 0x0002,   // Hide temperature
};

static constexpr const uint8_t EEPROM_LAYOUT_TAG = 1;
static constexpr const uint8_t RTC_RAM_LAYOUT_TAG = 1;
static constexpr const uint8_t USERS_LAYOUT_TAG = 1;

// Global setup (placed at begining of EEPROM)
struct GolbalSetup {
    // These fields (*_tag) must be first
    uint8_t eeprom_layout_tag; // Unique number, corresponded to current layout of EEPROM. If this tag differ from EEPROM_LAYOUT_TAG contents of EEPROm will be initialize from blank page.
    uint8_t rtc_ram_layout_tag; // Unique number, corresponded to current layout of RTC RAM. If this tag differ from RTC_RAM_LAYOUT_TAG contents of RTC RAM will be initialize from blank page.
    uint8_t users_layout_tag; // Unique number, corresponded to current layout of User Setup in EEPROM. If this tag differ from USERS_LAYOUT_TAG contents of ES_User, ES_UName and Template library in FP will be erased
    uint8_t reserved_tag;
    ///////////////
        
    uint16_t round_time; // Time between Play Rounds (In minutes)
    uint16_t options;   // Bitset of GlobalOptions
    
    uint8_t  time_sync; // Time between Time Sync (In hours)

    uint8_t  guard; // We writes here 0xFF - if we extand this structure later and seen 0xFF here after load it will meant that data after this field should be initialized
    
    void sync() const; // Save me to EEPROM
};
static_assert(sizeof(GolbalSetup) < 32*4, "GolbalSetup structure overflow");


// EEPROM layout (EEPROM size is 4K, 128 blocks by 32 bytes)
// Block number for different data
enum EEPROMShifts {
    ES_Global,                  // GolbalSetup (reserved space up to 4 blocks)
    ES_SSID = ES_Global + 4,    // SSID
    ES_Passwd,                  // Password (2 blocks)
    ES_Touch  = ES_Passwd+2,    // TouchSetup
    
    ES_TOP_MAX, // Marker, not a real block
    
    ES_User     = 80,   // UserSetup[32]    
    ES_UName    = 96    // UserName[32]  (char[32][32])
};
static_assert(ES_TOP_MAX < ES_User, "EEPROM overflow");

///////////////////////////////////////////////

//  RTC RAM data (up to 56 bytes)
enum WorkingStatus : uint8_t {
    WS_NotActive,   // Play Round not started
    WS_Pending,     // Play Round will be started an 'nest time'
    WS_Active       // Play Round now running
};

struct WorkingState {
    uint32_t last_round_time; // Time of last Play Round or time of start of Play Round
    uint32_t last_tsync_time; // Time of last TimeSync event
    uint32_t enabled_users; // Bitset of all users which can take Challenge right now
    WorkingStatus state; // Current status of Play Round
    uint8_t load_state[8]; // State of loading of all doors: <position-in-queue:3 bit><user-index:5 bit>. Value of FF means 'unloaded'

    uint8_t  guard; // We writes here 0xFF - if we extand this structure later and seen 0xFF here after load it will meant that data after this field should be initialized
    
    void sync() const; // Save me to RTC
};
static_assert(sizeof(WorkingState) <= 56, "RTC RAM Overflow");
////////////////////////////////////////////////

// Global run-time state
enum SystemState {
    SS_NotActive,   // Play Round not active
    SS_Pending,     // Play Round scheduled
    
    SS_Active,      // Play Round activated (this state not used by itself)
    
    SS_NotLoggedIn, // No user currently logged in by fingerprint
    SS_InvalidUser, // User logged in (if any) not valid here - not enabled, not included in Play Round, Play Rond done)
    SS_NotATime,    // Time of Round not come for logged in user
    SS_NotLoaded,   // No gift loiaded for logged in user yet
    
    SS_Challenge,   // Challenge activated for logged in user
    SS_Done         // Challenge end, door opened. Waiting for logout
};

extern SystemState system_state;
extern TouchSetup touch_setup;
extern int logged_in_user;
extern UserSetup current_user;
extern uint8_t current_user_name[32];
extern WorkingState working_state;
extern GolbalSetup global_setup;
extern uint8_t ssid[33];
extern uint8_t passwd[64];

void init_or_load_setup();

inline void login_user(int usr_index) 
{
    logged_in_user = usr_index;
    if (usr_index == -1) {current_user.clear(); current_user_name[0] = 0; return;}
    assert(usr_index >= 0 && usr_index < max_users);
    current_user.load(usr_index, current_user_name);
}
