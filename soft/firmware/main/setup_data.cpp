#include "common.h"

#include "setup_data.h"
#include "hadrware.h"
#include "ILI9327_Shield.h"
#include "activity.h"
#include "web_gadgets.h"

int logged_in_user = -1;

UserSetup current_user;
uint8_t current_user_name[33];
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

void zap_configs()
{
    init_rtc_data();
    init_eeprom_data();
    init_user_eeprom_data();
}

int WorkingState::get_loaded_gift(int user_index) // Returns Door index with gift fot User, or -1 if no gift loaded
{
    int pos = -1;
    uint8_t ord = 0xFF;
    for(int idx=0; idx < 8; ++idx)
    {
        uint8_t v = load_state[idx];
        if (v==0xFF) continue;
        if ((v&0x1F) != user_index) continue;
        if (v < ord) {pos = idx; ord = v;}
    }
    return pos;
}

int WorkingState::total_loaded_gift(int user_index)
{
    int result = 0;
    for(int idx=0; idx < 8; ++idx)
    {
        uint8_t v = load_state[idx];
        if (v != 0xFF && (v&0x1F) == user_index) ++result;
    }
    return result;
}

// Emit user name with gift order (if any). No quotes (just <User (N)>), Return true
// If requested slot is empty emit nothing and return false
bool WorkingState::write_user_name(Ans& ans, int index, bool add_quotes)
{
    uint8_t state = load_state[index];
    if (state == 0xFF)
    {
        if (add_quotes) ans << UTF8 << "null";
        return false;
    }
    auto uidx = state & 31;
    if (add_quotes) ans << UTF8 << "\"";
    ans << DOS << EEPROMUserName(uidx).dos();
    state >>= 5;
    if (state || working_state.get_loaded_gift(uidx) > 1) ans << UTF8 << " (" << (state+1) << ")";
    if (add_quotes) ans << UTF8 << "\"";
    return true;
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

bool UserSetup::load(int usr_index, uint8_t name[33])
{
    EEPROM::read(ES_User * EEPROM::page_size + sizeof(UserSetup)*usr_index, this, sizeof(UserSetup));
    if (name)
    {
        EEPROM::read((ES_UName + usr_index) * EEPROM::page_size, name, 32);
        name[32]=0;
    }
    if (empty()) return false;
    if (name) return (name[0] != 0 && name[0] != 0xFF);
    uint8_t n;
    EEPROM::read((ES_UName + usr_index) * EEPROM::page_size, &n, 1);
    return (n != 0 && n != 0xFF);
}

void UserSetup::save(int usr_index, uint8_t name[32]) const
{
    EEPROM::write(ES_User * EEPROM::page_size + sizeof(UserSetup)*usr_index, this, sizeof(UserSetup));
    if (name)  EEPROM::write((ES_UName + usr_index) * EEPROM::page_size, name, 32);
}

// Write user name with prefix/suffix, as directed by its rights (not implemented now)
void UserSetup::write_usr_name(Ans& ans, uint8_t name[33])
{
    ans << DOS << name;
}

uint32_t get_eeprom_users(int mask_to_test, int value_to_compare)
{
    uint32_t result = 0;
    for(int i=0; i<32; ++i)
    {
        UserSetup usr;
        if (!usr.load(i, NULL)) continue;
        if (mask_to_test && (usr.status & mask_to_test) != value_to_compare) continue;
        result |= 1 << i;
    }
    return result;
}

EEPROMUserName::EEPROMUserName(int user_index)
{
    user_index &= 31;
    buf.fill(0, 33);
    EEPROM::read((ES_UName + user_index) * EEPROM::page_size, buf.c_str(), 32);
}

const char* EEPROMUserName::utf8()
{
    if (buf.length() == 33)
    {        
        for(int idx = 0; idx < 32; ++idx)
        {
            char s = buf[idx];
            if (!s) break;
            buf.strcat(dos_to_utf8(s).b);
        }
        if (buf.length() == 33) buf.cat_fill(0, 1);
    }
    return buf.c_str() + 33;

}

// Return bit scale of filled templates for this user
uint8_t fge_get_filled_tpls(int usr_index)
{
    uint8_t buf[32];
    int err = Activity::FPAccess(NULL).access().readIndexTable(buf);
    if (err) return 0;// this is error, what to do?
    uint8_t result = buf[(usr_index>>1)&31];
    if (usr_index&1) result >>= 4;
    return result & 15;
}
