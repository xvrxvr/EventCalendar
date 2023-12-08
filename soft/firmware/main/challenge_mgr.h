#pragma once

#include "prnbuf.h"

class ChFile;

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

    void shuffle_challenges();

    // Retrun true if file was successfully read
    bool read_ch_file(ChFile& dst, int ch_index);
};

class ChFile {
    friend class ChallengeMgr;
    using P2 = std::pair<size_t, size_t>;
    std::unique_ptr<char[]> data;
    size_t data_size;
    P2 header_ptr;
    P2 body_ptr;
    P2 ans_ptr;
    P2 valid_ans_ptr;

    std::string_view get(P2 who) const {return {data.get()+who.first, who.second};}
public:

    std::string_view get_header() const {return get(header_ptr);}
    std::string_view get_body() const {return get(body_ptr);}
    std::string_view get_ans() const {return get(ans_ptr);}
    std::string_view get_valid_ans() const {return get(valid_ans_ptr);}
};



ChallengeMgr& challenge_mgr();
