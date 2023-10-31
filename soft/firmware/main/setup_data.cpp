#include "common.h"

#include "setup_data.h"
#include "hadrware.h"
#include "ILI9327_Shield.h"

SystemState system_state = SS_NotActive;
int logged_in_user = -1;

UserSetup current_user;
uint8_t current_user_name[32];
TouchSetup touch_setup;
WorkingState working_state;
GolbalSetup global_setup;

uint8_t ssid[33];
uint8_t passwd[64];

static void init_rtc_data()
{
    working_state = WorkingState{
        .last_round_time = 0,
        .enabled_users = 0,
        .state = WS_NotActive,
        .load_state = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .guard = 0xFF
    };
    working_state.sync();
}

static void init_eeprom_data()
{
    global_setup = GolbalSetup{
        .eeprom_layout_tag = EEPROM_LAYOUT_TAG,
        .rtc_ram_layout_tag = RTC_RAM_LAYOUT_TAG,
        .users_layout_tag = USERS_LAYOUT_TAG,
        .reserved_tag = 0xFF,
            
        .round_time = 60, // Time between Play Rounds (In minutes)
        .options = 0,   // Bitset of GlobalOptions

        .tz_shift = 0, // TZ is 0 (GMT)

        .guard = 0xFF
    };
    global_setup.sync();
    touch_setup.clear();
//    touch_setup.sync(); -- Write is not needed - we writes it after calibration

    uint8_t zero = 0;

    ssid[0] = 0;
    passwd[0] = 0;
    EEPROM::write(ES_SSID, zero);
    EEPROM::write(ES_Passwd, zero);
}

static void init_user_eeprom_data()
{
    UserSetup empty;
    uint8_t empty_name[32]={0};

    empty.clear();

    for(int i=0; i<max_users; ++i) empty.save(i, empty_name);

    fp_sensor.emptyLibrary();
}

static void load_rtc_data()
{
    RTC().read_ram(&working_state, 0, sizeof(working_state));
    // Check for 'guard' field in a future
}

#define LOAD_STRING(Page, Dst) do {EEPROM::read_pg(Page, Dst, sizeof(Dst)-1); Dst[sizeof(Dst)-1]=0; if (Dst[0] == 0xFF) Dst[0] = 0;} while(0)
static void load_eeprom_data()
{
    EEPROM::read(ES_Global, global_setup);
    // Check for 'guard' field in a future
    EEPROM::read(ES_Touch, touch_setup);
    LOAD_STRING(ES_SSID, ssid);
    LOAD_STRING(ES_Passwd, passwd);
}
#undef LOAD_STRING

void init_or_load_setup()
{
    uint8_t tags[3];
    EEPROM::read(0, tags, sizeof(tags));

    if (tags[1] != RTC_RAM_LAYOUT_TAG) init_rtc_data(); else load_rtc_data();
    if (tags[0] != EEPROM_LAYOUT_TAG) init_eeprom_data(); else load_eeprom_data();
    if (tags[2] != USERS_LAYOUT_TAG && tags[2] != 0xFF) init_user_eeprom_data();
    
    if (touch_setup.empty())
    {
        touch_setup.calibrate();
    }

    current_user.clear();
}

/////////////////////
void TouchSetup::sync() const // Save me to EEPROM
{
    EEPROM::write(ES_Touch, *this);
}

void GolbalSetup::sync() const // Save me to EEPROM
{
    EEPROM::write(ES_Global, *this);
}

void WorkingState::sync() const // Save me to RTC
{
    RTC().write_ram(this, 0, sizeof(*this));
}

void UserSetup::load(int usr_index, uint8_t name[32])
{
    EEPROM::read(ES_User * EEPROM::page_size + sizeof(UserSetup)*usr_index, this, sizeof(UserSetup));
    EEPROM::read((ES_UName + usr_index) * EEPROM::page_size, name, 32);
}

void UserSetup::save(int usr_index, uint8_t name[32]) const
{
    EEPROM::write(ES_User * EEPROM::page_size + sizeof(UserSetup)*usr_index, this, sizeof(UserSetup));
    EEPROM::write((ES_UName + usr_index) * EEPROM::page_size, name, 32);
}

