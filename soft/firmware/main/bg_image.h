#pragma once

class LCD;

class BGImage {
    std::vector<bool> bg_list, usr_bg_list;
    bool initialized = false;
    int bg_img_index = -1;

    void init();
public:

    void peek_next_bg_image();
    void draw(LCD&);
    int scan_bg_images(int& state);
    int create_new_bg_image();
};
