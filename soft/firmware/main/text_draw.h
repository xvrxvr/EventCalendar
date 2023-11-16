#pragma once

#include <array>
#include <optional>		
#include <regex>
#include <string_view>

#include "hadrware.h"
#include "box_draw.h"

namespace TextBoxDraw {

enum Align : uint8_t {
    A_None,
    A_Left,
    A_Center,
    A_Right
};

using Size = std::pair<int, int>;
using String = std::string_view;
using Align3 = std::array<int, 3>;
using Align3P = std::array<std::pair<int,int>, 3>;

class WordWrapError : public std::exception {
public:
    WordWrapError(const char*) {}
};

class OutOfBounds : public std::exception {
public:
    OutOfBounds(const char*) {}    
};

class Error : public std::exception {
public:
    Error(const char*) {}    
};


enum GlobOptions : uint16_t {
    GO_KbMask       = 0x000F,
    GO_KbEnglish    = 0x0001,
    GO_KbRussian    = 0x0002,
    GO_KbNumbers    = 0x0004,
    GO_KbSelector   = 0x0008,
    GO_BoxDirMask   = 0x0030,
    GO_BoxDirAuto   = 0,
    GO_BoxDirVert   = 0x0010,
    GO_BoxDirHor    = 0x0020,
    GO_LetSzMask    = 0x00C0,
    GO_LetSzAuto    = 0,
    GO_LetSzSmall   = 0x0040,
    GO_LetSzBig     = 0x0080,
    GO_WordWrap     = 0x0100,
    GO_FuzzyPercent = 0x0200,
    GO_CentrVert    = 0x0400,
    GO_CentrHor     = 0x0800,
    GO_EqSizeHor    = 0x1000,
    GO_EqSizeVert   = 0x2000
};


struct TextGlobalDefinition {
    uint16_t border_color = 0;
    uint16_t shadow_color = rgb(0x66,0x66,0x66);
    uint16_t fg_color = 0;  // FG color (rgb565 hex string)
    uint16_t bg_color = 0xAFD5;  // BG color (rgb565 hex string)
    GlobOptions glob_options = GlobOptions(GO_FuzzyPercent|GO_CentrVert|GO_CentrHor|GO_EqSizeHor|GO_EqSizeVert);

    uint8_t marging_v = 5;
    uint8_t marging_h = 5;
    uint8_t padding_v = 5;
    uint8_t padding_h = 5;
    uint8_t border_width = 1;
    uint8_t shadow_width = 5;
    uint8_t corner_r = 5;
    uint8_t fuzzy_dist = 30;    // Text distance for Fuzzy compare. Number (in symbols) or string in form <???>% - for percent of answer length (excluding digits)
    uint8_t min_age = 10;
    uint8_t max_age = 99;

#define OPT(name) return (glob_options & name) != 0
    bool word_wrap() const {OPT(GO_WordWrap);}
    bool fuzzy_is_percent() const {OPT(GO_FuzzyPercent);}
    bool center_vertical() const {OPT(GO_CentrVert);}
    bool center_horizontal() const {OPT(GO_CentrHor);}
    bool equal_size_vertical() const {OPT(GO_EqSizeVert);}
    bool equal_size_horizontal() const {OPT(GO_EqSizeHor);}
#undef OPT
#define OPT(mask) return GlobOptions(glob_options & mask)
    GlobOptions letter_size() const {OPT(GO_LetSzMask);}
    GlobOptions boxes_dir() const {OPT(GO_BoxDirMask);}
    GlobOptions keyb_type() const {OPT(GO_KbMask);}
#undef OPT

    // Setup from header. Return true if ok. Invalid fields inside header will not fail load process, but set field to zero values
    bool setup(const char* value);
};

class TextSegment {

    Size get_letter_size(int default_letter_size, int total_letters) const;

public:
    uint8_t letter_size=0;    // 0 -auto, 1,2 - defined size
    Align align = A_None;         // '' - no align, else < or > or #
    bool spacing = false;    // false - no spacing, true - spacing at each symbol
    uint16_t fg_color = 0;      // FG color
    uint16_t bg_color = 0;      // BG color (hex string)
    bool word_wrap = false;  // Enable word wrap (by spaces)
    int16_t x = 0;        // Coordinates for drawing
    int16_t y = 0;
    int16_t width = 0;    // Optional width to fit (in case of space insertion mode)

    String text;          // Line of text

    bool have_letter_size() const {return letter_size != 0;}

    // Return array of [w, h] for this segment.
    Size text_size(int default_letter_size=0) const
    {
        return get_letter_size(default_letter_size, text.size());
    }

    // Process '\...' control code. 'symbol' is a pure code (without '\')
    void process_ctrl(const String& symbol);

    // Clone all control from current to new one, except Align and text itself
    TextSegment clone(const String& text = {})
    {
        TextSegment result(*this);
        result.text = text;
        result.align = A_None;
        return result;
    }

    void reset()
    {
        text = String();
        align = A_None;
    }

    void reset(const TextGlobalDefinition& gd)
    {
        text = String();
        align       = A_None;
        letter_size = gd.letter_size();
        spacing     = false;
        fg_color    = gd.fg_color;
        bg_color    = gd.bg_color;
        word_wrap   = gd.word_wrap();
    }


    // Create full duplicate
    TextSegment dup() {return TextSegment(*this);}

    TextSegment dup(const String& text)
    {
        TextSegment result(*this);
        result.text = text;
        return result;
    }

    // Split this item in 2 (if possible) on required 'width'. Returns new item (with limited width) and leave rest of string in this one.
    // Returns new item or 'undefined' if can't split
    std::optional<TextSegment> trim_by_width(int16_t width, int default_letter_size=0);

    // Method to draw current object to LCD
    void draw_to_canvas(LCD& lcd, bool last_in_line, int default_letter_size=0);

};

class TextLine {
    // Assign positions for each slice of text - version when Align defined for this line
    void eval_position_aligned(int x, int y, int width, int height, int default_letter_size);

    // Assign positions for each slice of text - version when Spacing defined for this line
    void eval_position_spacing(int x, int y, int width, int height, int default_letter_size);

    void check_word_wrap_enabled();

    TextLine(const std::vector<TextSegment>& org, const Align3& as) : line(org), align_segments(as) {}

public:

    std::vector<TextSegment> line;
    Align3 align_segments{-1, -1, -1};

    TextLine() {}

    void reset()
    {
        line.clear();
        align_segments.fill(-1);
    }

// was used only in Exceptions arguments - throw away
//    get raw_text() {return this.line.reduce( (acc, cur_value) => acc + cur_value.text, '');}

    bool have_letter_size() const
    {
        for(const auto& l: line)
        {
            if (l.have_letter_size()) return true;
        }
        return false;
    }

    // Return array of [w, h] for this line.
    Size text_size(int default_letter_size=0) const;

    TextLine clone(const std::vector<TextSegment>& new_line)
    {
        return TextLine(new_line, align_segments);
    }

    // Performs WordWrap to requested width. Fill array of lines ('target' parameter)
    // Raise exception if can't by some reason
    void word_wrap(std::vector<TextLine> &target, int width, int default_letter_size=0);

    // Assign positions for each slice of text
    void eval_position(int x, int y, int width, int default_letter_size=0);

    void add_segment(const TextSegment& text_segment);

    // Method to draw current object to LCD
    void draw_to_canvas(LCD& lcd, int default_letter_size=0)
    {
        for(int i = 0; i < line.size(); ++i) 
        {
            line[i].draw_to_canvas(lcd, i+1 == line.size(), default_letter_size);
        }
    }
};

struct TextsParserSelected;

class TextsParser {
    friend struct TextsParserSelected;

    const TextGlobalDefinition& global_definitions;
    TextLine cur_text_line;
    TextSegment cur_text_segment;
    std::vector<TextLine> text_lines;

    void parse_line(const String& line);

    void draw_one_box_centered_imp(LCD& lcd, int x, int y, int width, int height);

    void draw_one_box_imp(LCD& lcd, int x, int y, int width, int height);

    int need_ww(int width);

public:
    TextsParser(const TextGlobalDefinition& gl) : global_definitions(gl) {}
    TextsParser(TextsParser && from) : global_definitions(from.global_definitions) {text_lines = from.text_lines;}

    void operator=(TextsParser && from) {text_lines = from.text_lines;}

    void parse_text(const String& lines);

    bool have_letter_size() const 
    {
        for(const auto& element: text_lines)
        {
            if (element.have_letter_size()) return true;
        }
        return false;
    }

    // Return array of [w, h] for this block.
    Size text_size() const;

    // Performs word wrap, returns new instance of TextsParser (if wrap is successfull) or raise an exception
    TextsParser word_wrap(int width);

    // Assign positions for each line of text
    // Margins and Pads not taken in account - caller of this method should take care of it itself by adjusting x,y,width and height
    void eval_positions(int x, int y, int width, int height);

    // Evaluate minimal box size (external dims). Retuns as array
    Size min_box_size() const
    {
        const auto sz = text_size();
        const auto res = reserved_space();

        return {sz.first+res.first, sz.second+res.second};
    }

    // How much space reserved between Box outside boundary and text inside
    Size reserved_space() const;

    // Evaluate Box size and runs position evaluation
    // Returns array with Box definition [<width>, <height>]
    Size eval_box(int x, int y, int width = 0, int height = 0);

    // method to draw current TextParser to Canvas
    void draw_to_canvas(LCD& lcd)
    {
        for(auto& line: text_lines) line.draw_to_canvas(lcd, global_definitions.letter_size());
    }

    void draw_box_to_canvas(LCD& lcd, int x, int y, int width, int height)
    {
        BoxCreator box(width-global_definitions.shadow_width, height-global_definitions.shadow_width, global_definitions.corner_r, global_definitions.border_width, global_definitions.shadow_width);
        const uint16_t pallete[] = {0, global_definitions.bg_color, global_definitions.border_color, global_definitions.shadow_color};
        box.draw(lcd, x, y, pallete, true);
    }

    void draw_one_box_centered(LCD& lcd, int x=0, int y=0, int width=400, int height=240)
    {
        if (int w = need_ww(width - 2*global_definitions.marging_h)) word_wrap(w).draw_one_box_centered_imp(lcd, x, y, width, height);
        else draw_one_box_centered_imp(lcd, x, y, width, height);
    }

    void draw_one_box(LCD& lcd, int x, int y, int width, int height)
    {
        if (int w = need_ww(width)) word_wrap(w).draw_one_box_imp(lcd, x, y, width, height);
        else draw_one_box_imp(lcd, x, y, width, height);
    }

    void draw_selection_of_boxes(LCD& lcd, const std::vector<int>& sel_array, int x=0, int y=0, int width=400, int height=240);
};

struct TextsParserSelected {
    TextsParser obj;
    Size sz;

    TextsParserSelected(const TextGlobalDefinition& gd, const TextLine& tl, int width, int height);
    TextsParserSelected() = default;
};

} // namespace TextBoxDraw
