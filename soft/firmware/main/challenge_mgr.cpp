#include "common.h"

#include "challenge_mgr.h"
#include "hadrware.h"
#include "setup_data.h"
#include "web_gadgets.h"

#include <random>
#include <fcntl.h>
#include <sys/stat.h>

static const char* TAG = "challenge";

ChallengeMgr& challenge_mgr()
{
    static ChallengeMgr ret;
    return ret;
}

char* ChallengeMgr::ch_name(int ch_index) {return buf.printf("/data/ch.%d", ch_index).c_str();}

void ChallengeMgr::fill_user()
{
    for(auto& fl: files)
    {
        FILE* f = fopen(ch_name(fl.first), "r");
        if (!f) continue;
        const char* usr = find_hdr(f, 'u');
        if (usr) fl.second = atoi(usr);
        fclose(f);
    }
}

const char* ChallengeMgr::get_dos_name(int ch_index)
{
    FILE* f = fopen(ch_name(ch_index), "r");
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
    DIR *dir = opendir("/data/");
    if (!dir)
    {
        ESP_LOGE(TAG, "opendir('/data/') failed: %s", strerror(errno));
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

void ChallengeMgr::delete_challenge(int ch_index)
{
    unlink(ch_name(ch_index));
    for(size_t i=0; i<files.size(); ++i)
    {
        if (files[i].first == ch_index)
        {
            files.erase(files.begin()+i);
            break;
        }
    }
}

void ChallengeMgr::send_challenge(class Ans& ans, int ch_index)
{
    FILE* f = fopen(ch_name(ch_index), "r");
    if (!f) return;
    char* b = buf.fill(0, SC_FileBufSize).c_str();
    for(;;)
    {
        int sz = fread(b, 1, SC_FileBufSize, f);
        if (sz <= 0) break;
        ans.write_string_dos(b, sz);
        if (sz < SC_FileBufSize) break;
    }
    fclose(f);
}

ChallengeMgr::ChUpd ChallengeMgr::update_challenge(uint8_t* first_pack, size_t first_pack_size)
{
    int ch_index = -1;
    size_t delta = 0;
    if (first_pack[0] == 'i') // It should be
    {
        ch_index = strtol((char*)first_pack+1, NULL, 10);
        uint8_t* e = (uint8_t*)memchr(first_pack, '\n', first_pack_size);
        if (e)
        {
            ++e;
            delta = e-first_pack;
            first_pack_size -= delta;
            first_pack = e;
        }
    }
    if (ch_index == -1)
    {
        if (!files.empty())
        {
            std::sort(files.begin(), files.end());
            if (files.back().first != files.size() - 1) // We have some unused indexes inside - try to find them
            {
                for(int i=0; i<files.size(); ++i)
                {
                    if (files[i].first != i) // Index 'i' not occupied. We sort array before, so Ch indexes in filled part will be the same as array index
                    {
                        ch_index = i;
                        break;
                    }
                }
            }
        }
        if (ch_index == -1)
        {
            ch_index = files.size();
            files.push_back({ch_index, logged_in_user});
        }
    }
    FILE* f = fopen(ch_name(ch_index), "w");
    if (!f) return {NULL, -1, 0};
    fprintf(f, "u%d\n", logged_in_user);
    files[ch_index].second = logged_in_user;
    return {f, ch_index, fwrite(first_pack,1,first_pack_size, f) + delta};
}

void ChallengeMgr::shuffle_challenges()
{
//    std::minstd_rand rnd(esp_random());
//    std::shuffle(files.begin(), files.end(), rnd);
    // std::minstd_rand do not works properly :(
    for(int i=0; i<files.size(); ++i) std::swap(files[i], files[i + uint32_t(esp_random()) % (files.size()-i)]);
}

bool ChallengeMgr::read_ch_file(ChFile& dst, int ch_index)
{
    struct stat fs;
    int fd = open(ch_name(ch_index), O_RDONLY);
    if (fd==-1) return false;
    if (fstat(fd, &fs)) {close(fd); return false;}
    char* buffer = new char[fs.st_size+1];
    char* org = buffer;
    read(fd, buffer, fs.st_size);
    buffer[fs.st_size] = 0;
    close(fd);

    dst.data.reset(buffer);
    dst.data_size = fs.st_size;
    
    buffer = strstr(buffer, "\nV");
    if (!buffer)
    {
        ESP_LOGE(TAG, "V? mark not found in file");
        return false;
    } 
    ++buffer;
    if (memcmp(buffer, "V1 ", 3))
    {
        ESP_LOGE(TAG, "V1 expected in file");
        return false;
    }
    buffer+=3;
    auto body_lines = strtoul(buffer, &buffer, 10);

    char* prev_buf;
    auto nxt_line = [&]() { prev_buf = buffer; char* n = strchr(buffer, '\n'); if (n) buffer = n+1; else buffer += strlen(buffer);};

    // Header
    while(isspace(*buffer)) ++buffer;    
    dst.header_ptr.first = buffer - org;
    nxt_line();
    dst.header_ptr.second = buffer - prev_buf - 1; // Adjust for \n at end
    buffer[-1] = 0; // Terminate by zero

    // Body
    dst.body_ptr.first = buffer - org;
    char* pb = buffer;
    while(body_lines--) nxt_line();
    dst.body_ptr.second = buffer - pb - 1;

    // Answers
    dst.ans_ptr.first = buffer - org;
    dst.ans_ptr.second = fs.st_size - dst.ans_ptr.first;

    // Valid answer - first line of Ans
    dst.valid_ans_ptr.first = buffer - org;
    if (auto end = strchr(buffer, '\n')) dst.valid_ans_ptr.second = end - buffer;
    else dst.valid_ans_ptr.second = strlen(buffer);

    return true;
}
