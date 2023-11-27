#include "common.h"
#include "bg_image.h"
#include "activity.h"
#include "web_gadgets.h"

#include <rom/tjpgd.h>

BGImage bg_images;

static const char* TAG = "BgImages";

void BGImage::init()
{
    if (initialized) return;
    initialized = true;

    DIR *dir = opendir("/data/");
    if (!dir)
    {
        ESP_LOGE(TAG, "opendir('/data/') failed: %s", strerror(errno));
        return;
    }
    while (auto entry = readdir(dir))
    {
#define PREF "bg."        
        if (entry->d_type == DT_REG && memcmp(entry->d_name, PREF, sizeof(PREF)-1)==0) // This is bg.<n>.jpg file. N started from 0
        {
            char* e;
            int v = strtoul(entry->d_name + sizeof(PREF)-1, &e, 10);
            if (strcmp(e, ".jpg") == 0)
            {
                if (bg_list.size() <= v) bg_list.resize(v+1);
                bg_list[v] = true;
            }
        }
#undef PREF        
    }
    closedir(dir);
    usr_bg_list.resize(bg_list.size());
}

void BGImage::peek_next_bg_image()
{
    init();
    if (bg_list.empty()) {bg_img_index = -1; return;}
    int total = 0;
    for(size_t i=0; i< bg_list.size(); ++i)
    {
        if (bg_list[i] && !usr_bg_list[i]) ++total;
    }
    if (!total)
    {
        usr_bg_list.clear();
        usr_bg_list.resize(bg_list.size());
        for(size_t i=0; i< bg_list.size(); ++i)
        {
            if (bg_list[i]) ++total;
        }
        if (!total) // Strange - all is empty. Reset everytning
        {
            usr_bg_list.clear();
            bg_list.clear();
            bg_img_index = -1; 
            return;
        }
    }
    size_t idx = esp_random() % total;
    for(int i=0; i<bg_list.size(); ++i)
    {
        if (bg_list[i] && !usr_bg_list[i])
        {
            if (!idx--) {usr_bg_list[i] = true; bg_img_index = i; return;}
        }
    }
    assert(false && "BG index is lost???");
}

struct JPEGData {
    FILE* stream;
    LCD* lcd;
};

static unsigned int jpeg_decode_in_cb(JDEC *dec, uint8_t *buff, unsigned int nbyte)
{
    assert(dec != NULL);
    JPEGData* data = (JPEGData *)dec->device;
    assert(data != NULL);
    if (buff) return fread(buff, 1, nbyte, data->stream);
    fseek(data->stream, nbyte, SEEK_CUR);
    return nbyte;
}

static unsigned int jpeg_decode_out_cb(JDEC *dec, void *bitmap, JRECT *rect)
{
    assert(dec != NULL);
    JPEGData* data = (JPEGData *)dec->device;
    assert(data != NULL);
    data->lcd->draw_rgb888(rect->left, rect->top, rect->right, rect->bottom, (uint8_t*)bitmap);
    return 1;
}

static const char bg_fmt[] = "/data/bg.%d.jpg";

#define JPEG_WORK_BUF_SIZE  3100    /* Recommended buffer size; Independent on the size of the image */

void BGImage::draw(LCD& lcd)
{
    init();
    durty=false;
    if (bg_img_index == -1)
    {
        peek_next_bg_image();
        if (bg_img_index == -1)
        {
            lcd.DRect(0, 0, LCD::MAX, LCD::MAX, 0);
            return;
        } 
    }

    Prn b;
    b.printf(bg_fmt, bg_img_index);

    std::unique_ptr<FILE, decltype(&fclose)> stream(fopen(b.c_str(), "rb"), &fclose);
    
    if (!stream)
    {
        ESP_LOGE(TAG, "Can't open BG image file '%s'\n", b.c_str());
        return;
    }

    JDEC JDEC;
    std::unique_ptr<uint8_t[]> workbuf(new uint8_t[JPEG_WORK_BUF_SIZE]);

    JPEGData data {
        .stream = stream.get(),
        .lcd = &lcd
    };

    /* Prepare image */
    auto err = jd_prepare(&JDEC, jpeg_decode_in_cb, workbuf.get(), JPEG_WORK_BUF_SIZE, &data);
    if (err)
    {
        ESP_LOGE(TAG, "JPEG decoder.prepare error %d\n", err);
    }
    else
    {
        err = jd_decomp(&JDEC, jpeg_decode_out_cb, 0);
        if (err)
        {
            ESP_LOGE(TAG, "JPEG decoder.decomp error %d\n", err);
        }
    }
}

int BGImage::scan_bg_images(int& state)
{
    init();
    if (state == -1 || bg_list.empty()) {state = -1; return -1;}
    for(; state < bg_list.size(); ++state)
    {
        if (bg_list[state]) return state++;
    }
    state = -1; 
    return -1;
}

int BGImage::create_new_bg_image()
{
    init();
    for(int i=0 ;i < bg_list.size(); ++i)
    {
        if (!bg_list[i]) {bg_list[i] = true; return i;}
    }
    bg_list.push_back(true);
    usr_bg_list.push_back(false);
    return bg_list.size() - 1;
}

void BGImage::delete_bg_image(int idx)
{
    unlink(Prn().printf(bg_fmt, idx).c_str());
    if (idx < bg_list.size())
    {
        bg_list[idx] = false;
        usr_bg_list[idx] = false;
    }
}

FILE* BGImage::open_image(int idx, const char* mode) const
{
    return fopen(Prn().printf(bg_fmt, idx).c_str(), mode);
}

void BGImage::send_bg_image(class Ans& ans, int index) const
{
    FILE* f = open_image(index, "r");
    if (!f) {ans.send_error(HTTPD_404_NOT_FOUND, "Picture not found?"); return;}
    Prn buf;
    char* b = buf.fill(0, SC_FileBufSize).c_str();
    for(;;)
    {
        int sz = fread(b, 1, SC_FileBufSize, f);
        if (sz <= 0) break;
        ans.write_string_utf8(b, sz);
        if (sz < SC_FileBufSize) break;
    }
    fclose(f);
}
