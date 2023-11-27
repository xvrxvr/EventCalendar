#pragma once

class LCD;

class BGImage {
    std::vector<bool> bg_list, usr_bg_list; // bg_list - all existent BG files, usr_bg_list - all BG shown in this run
    bool initialized = false;
    int bg_img_index = -1;
    bool durty = false;

    void init();
public:

    void peek_next_bg_image();
    void draw(LCD&);
    int scan_bg_images(int& state);
    int create_new_bg_image();
    void delete_bg_image(int);
    FILE* open_image(int, const char* mode) const;

    bool is_durty() const {return durty;}
    void set_durty() {durty=true;}
    void send_bg_image(class Ans&, int) const;
};

extern BGImage bg_images;
