#pragma once

#include "prnbuf.h"

class ChallengeMgr {
    using ChInfo = std::pair<uint8_t, uint8_t>; // pair is <file-index,user-index>
    Prn buf;
    std::vector<ChInfo> files;

    void scan();
    void fill_user();

    int find_index(int file_index) const
    {
        for(int idx=0; idx<files.size(); ++idx)
        {
            if (files[idx].first == file_index) return idx;
        }
        return -1;
    }

    const char* find_hdr(FILE*, char);

    // Returns file name fo Challenge file. Use 'buf'
    char* ch_name(int ch_index);

public:
    ChallengeMgr() {scan(); fill_user();}

    const std::vector<ChInfo>& all_data() const {return files;}

    int get_usr_id(int ch_index) 
    {
        int idx = find_index(ch_index);
        return idx == -1 ? -1 : files[idx].second;
    }
    const char* get_dos_name(int ch_index);

    void delete_challenge(int ch_index);
    void send_challenge(class Ans&, int ch_index);
    
    struct ChUpd {
        FILE* file;
        int ch_index;
        size_t processed_size;
    };
    ChUpd update_challenge(uint8_t* first_pack, size_t first_pack_size);
};

ChallengeMgr& challenge_mgr();