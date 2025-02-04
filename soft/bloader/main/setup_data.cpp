#include "common.h"

#include "setup_data.h"
#include "hadrware.h"


static const char TAG[] = "setup";

uint8_t ssid[33];
uint8_t passwd[64];

#define LOAD_STRING(Page, Dst) do {EEPROM::read_pg(Page, Dst, sizeof(Dst)-1); Dst[sizeof(Dst)-1]=0; if (Dst[0] == 0xFF) Dst[0] = 0;} while(0)
void init_or_load_setup()
{
    LOAD_STRING(ES_SSID, ssid);
    LOAD_STRING(ES_Passwd, passwd);
}
#undef LOAD_STRING
