#pragma once

#include "grid_mgr.h"

enum DoorGridOptions : uint8_t {
    DGO_CloseLabelTopRight  = 0x01,   // Put 'Close' label in Top Right corner
    DGO_CloseLabelMiddle    = 0x02,   // Put 'Close' label in Middle cell
    DGO_CloseLabel2ndTry    = 0x04,   // Show up 'Close' label only after first Door open
    DGO_2Stage              = 0x08,   // Differentiate state before first Open Door and after

    DGO_AnyFinger           = 0x10,   // Accept any fingerprint
    DGO_Finger              = 0x20,   // Accept known fingerprint (and not current user)

    DGO_ResLeft             = 0x40,   // Reserved left pane for User usage

    DGOProfile_OpenDoor = DGO_CloseLabelTopRight|DGO_CloseLabel2ndTry|DGO_2Stage|DGO_Finger,
    DGOProfile_GiftLoad = DGO_CloseLabelMiddle|DGO_AnyFinger|DGO_ResLeft
};

class DoorGrid : public GridManager::Grid {
    enum Stage {
        S_WaitForOpen,  // Initial wait for possibility to open door_available
        S_Ready,        // Ready to open door
        S_WaitForReopen, // Wait for possibility to reopen door
        S_ReadyToReopen // Door can be reopened
    } stage;

    char txt[3]; // Downcounter. Up to 2 symbols possible
    DoorGridOptions options;
    bool ico_active = false;

    // Detect Stage and fill 'stage' member
    void detect_stage();

    // Setup active Door cell based on 'stage'
    void setup_cell();

    // Do detect_stage/setup_cell/update
    void sync();

    void draw_icon(LCD& lcd);

    virtual const char* get_text_dos(const GridManager::MiniCell& cell) override;

    bool is_close_icon(const Action&);

public:
    DoorGrid(DoorGridOptions options);
    void init(uint8_t active_doors);

    virtual const char* get_user_text_dos(int door_index) = 0;

    void open_physical_door(int index);
    void set_active_doors(uint8_t active_doors, bool do_sync=true);
    void update_doors_texts(uint8_t doors);

    uint32_t activity_get_actions();
    uint32_t activity_process(Activity&);

/*
    void init()
    {
        auto rc = enc[door_index];
        set_cell_box(rc.row, rc.col, 1);
        detect_stage();
        setup_cell();
        update_scene(Lcd(), true);
    }

    uint32_t run_activity()
    {
        detect_stage();
        setup_cell();
        update(Lcd());
        if (stage == S_WaitForOpen || stage == S_WaitForReopen) return run_dc_activity();
        else return run_stable_activity();
    }

    void update_scene(LCD& lcd, bool first_entry = false) 
    {
        bg_images.draw(lcd);
        if (first_entry) set_coord(lcd, GridManager::Rect{0, 0, RES_X, RES_Y});
        else update_all(lcd);
        lcd.set_bg(0);
        if (!door_not_opened_yet) draw_icon(lcd);
    }
*/
};

