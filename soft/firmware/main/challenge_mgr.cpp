#include "common.h"

#include <miniz.h>

#include "challenge_mgr.h"
#include "hadrware.h"
#include "setup_data.h"

static const char* TAG = "challenge";

ChallengeMgr& challenge_mgr()
{
    static ChallengeMgr ret;
    return ret;
}

void ChallengeMgr::fill_user()
{
    uint32_t crcs[32] = {};

    const auto find = [&](const char* usr_name) {
        uint32_t my_crc = mz_crc32(0, (const unsigned char *)usr_name, strlen(usr_name));
        for(int i=0; i<32; ++i) if (crcs[i] == my_crc) return i;
        return -1;
    };

    for(int idx=0; idx<32; ++idx)
    {
        EEPROMUserName name(idx);
        const char* n = name.dos();
        if (n[0] && n[0] != 0xFF) crcs[idx] = mz_crc32(0, (const unsigned char *)n, strlen(n));
    }

    for(auto& fl: files)
    {
        char* fname = buf.printf("/ch.%d", fl.first).c_str();
        FILE* f = fopen(fname, "r");
        if (!f) continue;
        const char* usr = find_hdr(f, 'u');
        fl.second = find(usr);
        fclose(f);
    }
}

const char* ChallengeMgr::get_dos_name(int ch_index)
{
    char* fname = buf.printf("/ch.%d", ch_index).c_str();
    FILE* f = fopen(fname, "r");
    if (!f) return "";
    const char* title = find_hdr(f, 't');
    fclose(f);
    return title;
}

static constexpr int max_tagget_len = 128;

const char* ChallengeMgr::find_hdr(FILE* file, char tag)
{
    if (buf.length() < max_tagget_len) buf.fill(0, max_tagget_len);

    while(fgets(buf.c_str(), max_tagget_len, file))
    {
        buf[max_tagget_len-1] = 0;
        if (buf[0] == tag)
        {
            int l = strlen(buf.c_str());
            if (l && buf[l-1] == '\n') buf[l-1] = 0;
            return buf.c_str()+1;
        }
        if (buf[0] == 'V') return NULL;
    }
    return NULL;
}

void ChallengeMgr::scan()
{
    DIR *dir = opendir("/");
    if (!dir)
    {
        ESP_LOGE(TAG, "opendir('/') failed: %s", strerror(errno));
        return;
    }
    while (auto entry = readdir(dir))
    {
#define PREF "ch."
        if (entry->d_type == DT_REG && memcmp(entry->d_name, PREF, sizeof(PREF)-1)==0) // This is ch.<n> file. N started from 0
        {
            char* e;
            int v = strtoul(entry->d_name + sizeof(PREF)-1, &e, 10);
            files.push_back(ChInfo{v, 0xFF});
        }
#undef PREF        
    }
    closedir(dir);
}
