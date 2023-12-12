#pragma once

namespace TextBoxDraw {
struct TextGlobalDefinition;
}

enum AnimationType {
    AT_None,        // No animation, just set color to <color_from>
    AT_One,         // Animate <color_from> -> <color_to> once
    AT_Pulse,       // Animate <color_from> -> <color_to> -> <color_from> one time
    AT_Periodic,    // Animate <color_from> -> <color_to> many times
    AT_Triange      // Animate <color_from> -> <color_to> -> <color_from> many times
};

struct AnimationSetup {
    AnimationType type;     // Type of animation
    uint16_t color_from;    // Start fg color
    uint16_t color_to;      // End fg color
    uint16_t length;        // Length of animation in ticks
};

struct SizeDef {
    int max_text_length;
    int max_text_lines;
    int max_icons_lines;
    int max_icons_count;

    std::pair<int16_t, int16_t> get_size(bool double_size_letter, int title_length) const;
};

struct Animation {
    AnimationSetup anim{};
    int stage = -1;
    int tick = 0;
    int icon_index = 0;

    void abort() {stage = -1; tick=0;}
    bool is_active() const {return stage != -1;}
    void assign(const AnimationSetup& a, int idx) {anim = a; tick=0; stage=0; icon_index=idx;}

    uint16_t color(bool raising_slope);
    uint16_t animate(bool do_tick);
};

class AnimatedPannel {
    static constexpr int MaxLines = 4;

    struct LineDef {
        std::unique_ptr<char[]> text; // NULL in 'text' - Icons line
    } lines[MaxLines];

    Animation main_anim, oob_anim;

    const TextBoxDraw::TextGlobalDefinition& bdef;
    int16_t box_width, box_height; // Sizes of box
    int16_t text_x, text_y; // Position of title line
    int16_t text_w; // Width of text arrea
    bool use_doube_size_letters = false;
    bool first_draw = true;
    int total_lines = 0, prev_total_lines = 0, start_line_to_redraw = 0;

    int icons_line = -1;
    uint32_t* icon = NULL;
    int icon_count;
    uint16_t icon_color;
    int16_t icon_y;

    std::unique_ptr<char[]> title;

    void draw_text(class LCD& lcd, const char* msg, int y);
    void draw_icons(class LCD& lcd);

    int sym_w() const {return use_doube_size_letters ? 16 : 8;}
    int sym_h() const {return sym_w() * 2;}

    // Emit text in one/double size and return text size in pixels
    int lcd_text(class LCD& lcd, const char* msg, int x, int y);

    void animate(class LCD&, Animation&, bool);

public:
    AnimatedPannel(const char* title_utf8, const SizeDef&, const TextBoxDraw::TextGlobalDefinition* box_def = NULL);

    void body_reset();
    void add_text_utf8(const char*, ...);
    void add_icons(uint32_t* icon, int count, uint16_t color); // Icons - 32x32 pixels
    void body_draw(class LCD&);

    AnimatedPannel& animate_icon(int icon_index, const AnimationSetup& setup, bool override=false);

    void tick(class LCD&, bool step=true);

    void body_draw();
    void tick(bool step=true);
};
