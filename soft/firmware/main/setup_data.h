#pragma once

#include "prnbuf.h"

// This module contains various setup data thet holds in EEPROM and RTC RAM

static constexpr int max_users = 32;

// Options for User
enum UserOptions {
//    UO_Admin                = 0x0001,   // This User is Admin - Usert is Admin if any bits here is set

    UO_CanManageBG          = 0x0001,   // This Admin can add/delete/disable background images
    UO_CanSetup             = 0x0002,   // This Admin can do system setup - touch calibration, full system reset

    UO_CanStartRound        = 0x0004,   // This Admin can start/stop Play Round
    UO_CanOpenDoors         = 0x0008,   // This Admin can forcibly open any door
    UO_CanLoadGifts         = 0x0010,   // This Admin can load gifts
    UO_CanSetupRoundTime    = 0x0020,   // This Admin can change Round time
    UO_CanManageChallenge   = 0x0040,   // This Admin can add/delete/edit quize questions

    UO_CanEditUser          = 0x0080,   // This Admin can edit another User definition
    UO_CanAddRemoveUser     = 0x0100,   // This Admin can add or remove Users
    UO_CanAddRemoveAdmin    = 0x0200,   // This Admin can add or remove another Admint
    UO_CanDisableUser       = 0x0400,   // This Admin can temporary disable/enable user
    UO_CanHelpUser          = 0x0800,   // This Admin can help another User to bypass a challenge (by logging in and pressing button on WEB interface)

    UO_CanViewFG            = 0x1000,   // This Admin can view FingerPrint status
    UO_CanEditFG            = 0x2000    // This Admin can edit FingerPrints
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
    
    bool load(int usr_index, uint8_t name[33]); // Return true if *this is not empty
    void save(int usr_index, uint8_t name[32]) const;

    bool empty() const {return options == 0xFFFF && status == 0xFF && priority == 0xFF && age == 0xFF;}
    void clear() {memset(this, 0xFF, sizeof(*this));}
    void write_usr_name(class Ans&, uint8_t name[33]);
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
    GO_HideTemp     = 0x0002,   // Hide temperature
};

static constexpr const uint8_t EEPROM_LAYOUT_TAG = 2;
static constexpr const uint8_t RTC_RAM_LAYOUT_TAG = 2;
static constexpr const uint8_t USERS_LAYOUT_TAG = 1;

enum LogSetupOptions {
    LSO_DefLogLevelMask = 0x07,
    LSO_UseIP           = 0x08,
    LSO_UseUART         = 0x10,
    LSO_SoftLimit       = 0x20  // Soft limit to allocated memory in Log Print buffer (buffer has no limit
};

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
    
    int8_t tz_shift; // Timezone shift (in 15 minutes quantities)
//    uint8_t  guard; // We writes here 0xFF - if we extand this structure later and seen 0xFF here after load it will meant that data after this field should be initialized

    // *** New part - Log Setup ***
    uint8_t  log_setup_options; // Bitset of LogSetupOptions (FF value is invalid)
    uint8_t  log_memsize_limit; // In 256 bytes increments
    uint8_t  log_reserved;
    uint32_t log_ip;

    uint8_t guard; // We writes here 0xFF - if we extend this structure later and seen 0xFF here after load it will meant that data after this field should be initialized

    
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

    ES_UsedQ,                   // Used questions (1 block per user)
    ES_UsedQNext = ES_UsedQ + 32, // Marker
    
    ES_TOP_MAX, // Marker, not a real block
    
    ES_User     = 80,   // UserSetup[32]    
    ES_UName    = 96    // UserName[32]  (char[32][32])
};
static_assert(ES_TOP_MAX < ES_User, "EEPROM overflow");

///////////////////////////////////////////////

//  RTC RAM data (up to 56 bytes)
enum WorkingStatus : uint8_t {
    WS_NotActive,   // Play Round not started
    WS_Pending,     // Play Round will be started an 'next time'
    WS_Active       // Play Round now running
};

struct WorkingState {
    uint32_t last_round_time; // Time of last Play Round or time of start of Play Round
    uint32_t enabled_users; // Bitset of all users which can take Challenge right now
    WorkingStatus state; // Current status of Play Round
    uint8_t load_state[8]; // State of loading of all doors: <position-in-queue:3 bit><user-index:5 bit>. Value of FF means 'unloaded'

    uint8_t  guard; // We writes here 0xFF - if we extend this structure later and seen 0xFF here after load it will meant that data after this field should be initialized
    
    void sync() const; // Save me to RTC
    int get_loaded_gift(int user_index); // Returns Door index with gift fot User, or -1 if no gift loaded
    int total_loaded_gift(int user_index);
    void unload_gift(int door_index); // Remove gift from requested door. Update Gifts indexes for user whose gift was removed

    // Emit user name with gift order (if any). Return true
    // If requested slot is empty - emit nothing and return false
    // If 'add_quotes' is true writes JSON complaint representation (quotes around name and 'null' if slot is empty)
    bool write_user_name(class Ans&, int slot_index, bool add_quotes);


    // Return true if 'guest' type of game active
    static bool is_guest_type();
};
static_assert(sizeof(WorkingState) <= 56, "RTC RAM Overflow");
////////////////////////////////////////////////

extern TouchSetup touch_setup;
extern int logged_in_user;
extern UserSetup current_user;
extern uint8_t current_user_name[33];
extern WorkingState working_state;
extern GolbalSetup global_setup;
extern uint8_t ssid[33];
extern uint8_t passwd[64];

void init_or_load_setup();
void zap_configs();

bool login_user(int usr_index);

inline void login_superuser()
{
    logged_in_user = -2;
    strcpy((char*)current_user_name, "Supervisor");
    memset(&current_user, 0, sizeof(current_user));
    current_user.options = 0xFFFF;
    current_user.priority = 0xFF;
}

// Return bitset of all filled Users (from EEPROM)
// If mask_to_test not zero bitset will be filtered by field UserSetup::status - if will be masked by mask_to_test and compared with 'value_to_compare'
//  Only entries passed test will be included in output bitset
uint32_t get_eeprom_users(int mask_to_test=0, int value_to_compare=0);

class EEPROMUserName {
    Prn buf;
public:
    EEPROMUserName(int user_index);

    const char* utf8();
    const char* dos() {return buf.c_str();}
};

class FPLib {
    uint8_t buf[32];
public:
    FPLib();

    uint8_t operator[](int);
    void sanitize();
};

// Return bit scale of filled templates for this user
inline uint8_t fge_get_filled_tpls(int usr_index) {return FPLib()[usr_index];}

// Delete FG. index is <User-index>*4 + <FG-index-in-lib>
void do_fg_del(int index, uint8_t count=1);

// November 1, 2023 0:00:00
static constexpr time_t timestamp_shift = 1698796800ull;

// Convert time stamp to UTC time
inline time_t ts2utc(uint32_t tm) {return tm+timestamp_shift;}
// Convert UTC to timestamp
inline uint32_t utc2ts(time_t ts) {return ts-timestamp_shift;}

// Convert time stamp value to text (in UTF8)
inline const char* ts_to_string(uint32_t tm)
{
    time_t ts = ts2utc(tm) + global_setup.tz_shift*(15*60);
    char* result = asctime(gmtime(&ts));
    if (char* e=strchr(result, '\n')) *e=0;
    return result;
}

void start_game();
