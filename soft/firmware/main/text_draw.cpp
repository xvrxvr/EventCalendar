#include "common.h"
#include "text_draw.h"

namespace TextBoxDraw {

const TextBoxDraw::TextGlobalDefinition default_text_global_definition;

uint8_t unbase64_digit(const char val)
{
    if (val >= '0' && val <= '9') return val - '0';
    if (val >= 'A' && val <= 'Z') return val - 'A' + 10;
    if (val >= 'a' && val <= 'z') return val - 'a' + 36;
    switch(val)
    {
        case '+': return 62;
        case '-': return 63;
        default: return 0;
    }
}

inline uint16_t unhex(const char* str, int length)
{
    uint16_t acc = 0;
    while(length--)
    {
        acc <<= 4;
        acc |= unbase64_digit(*str++);
    }
    return acc;
}

// Setup from header. Return true if ok. Invalid fields inside header will not fail load process, but set field to zero values
bool TextGlobalDefinition::setup(const char* value)
{
    marging_v    = unbase64_digit(value[0]);
    marging_h    = unbase64_digit(value[1]);
    padding_v    = unbase64_digit(value[2]);
    padding_h    = unbase64_digit(value[3]);
    border_width = unbase64_digit(value[4]);
    shadow_width = unbase64_digit(value[5]);
    corner_r     = unbase64_digit(value[6]);
    fuzzy_dist   = unbase64_digit(value[7]);
    min_age      = unhex(value+8, 2);
    max_age      = unhex(value+10, 2);

    glob_options = GlobOptions(unhex(value+12, 4));

    const char* p = strchr(value, ' ');
    if (!p) return false;
    ++p;

#define COLOR(name) \
    if (*p == '-') {name = 0; ++p;}         \
    else {name = unhex(p, 4); p+=4;}        \
    if (*p != ',') return false;            \
    p++

    COLOR(bg_color);
    COLOR(border_color);
    COLOR(shadow_color);
    COLOR(fg_color);
#undef COLOR
    return true;
}

inline void trim_front(String& text)
{
    while(!text.empty() && isspace(text.front())) text.remove_prefix(1);
}

inline void trim_back(String& text)
{
    while(!text.empty() && isspace(text.back())) text.remove_suffix(1);
}

Size TextSegment::get_letter_size(int default_letter_size, int total_letters) const
{
    if (letter_size) default_letter_size = letter_size;
    if (!default_letter_size) default_letter_size = 1;
    return {total_letters * (default_letter_size == 2 ? 16 : 8), default_letter_size == 2 ? 32 : 16};
}

// Process '\...' control code. 'symbol' is a pure code (without '\')
void TextSegment::process_ctrl(const String& symbol) 
{
   switch(symbol[0])
   {
       case '1': letter_size = 1; break;
       case '2': letter_size = 2; break;
       case '<': align = A_Left; break;
       case '>': align = A_Right; break;
       case '#': align = A_Center; break;
       case 's': spacing = true; break;
       case 'w': word_wrap = true; break;
       case 'c': fg_color = unhex(symbol.data()+1, 4); break;
       case 'b': bg_color = unhex(symbol.data()+1, 4); break;
   }
}

// Split this item in 2 (if possible) on required 'width'. Returns new item (with limited width) and leave rest of string in this one.
// Returns new item or 'undefined' if can't split
std::optional<TextSegment> TextSegment::trim_by_width(int16_t width, int default_letter_size)
{
    const auto let_width = get_letter_size(default_letter_size, 1).first;
    auto prev_good_index = String::npos;
    
    for(size_t index = 0; (index = text.find(' ', index)) != String::npos; ++index)
    {
        if (let_width*index > width) // Split here
        {
            if (prev_good_index == String::npos) return {};
            String result = text.substr(0, prev_good_index);
            text.remove_prefix(prev_good_index);
            trim_front(text);
            trim_back(result);
            return dup(result);
        }
        prev_good_index = index;
    }
    return {}; // Should not pass here
}

// Method to draw current object to LCD
void TextSegment::draw_to_canvas(LCD& lcd, bool last_in_line, int default_letter_size)
{
    const auto sz = get_letter_size(default_letter_size, 1).first;
    const bool big_font = sz > 8;

    lcd.set_fg(fg_color);
    lcd.set_bg(bg_color);

    if (spacing && width)
    {
        const int target_width = width;
        const int my_width = sz * text.size();

        int current_width = 0;
        auto xx = x;

        for(const auto sym: text)
        {
            if (big_font) lcd.text2(&sym, xx, y, 1); 
            else lcd.text(&sym, xx, y, 1);
            current_width += sz; 
            xx = x + target_width * current_width / my_width;
        }
    }
    else
    {
        if (big_font) lcd.text2(text.data(), x, y, text.size()); 
        else lcd.text(text.data(), x, y, text.size());
    }
}


// Assign positions for each slice of text - version when Align defined for this line
void TextLine::eval_position_aligned(int x, int y, int width, int height, int default_letter_size)
{
    Align3P aligns_by_types = {Size{-1, -1}, Size{-1, -1}, Size{-1, -1}}; // Aligns rearranged by types. Value - {-1, -1} (no segmnent) or {<start index>, <length>} (for segment)

    const auto write_line = [&, this](int al_type, int xx) {
        for(int i = 0; i < aligns_by_types[al_type].second; ++i)
        {
           auto & l = line[i+aligns_by_types[al_type].first];
           const auto sz = l.text_size(default_letter_size);
           l.x = xx; l.y = y + height - sz.second;
           xx += sz.first;
        }
        return xx;
    };

    const auto size_line = [&, this](int al_type) {
        int total = 0;
        for(int i = 0; i < aligns_by_types[al_type].second; ++i) 
            total += line[i+aligns_by_types[al_type].first].text_size(default_letter_size).first;
        return total;
    };

    for(int i=0; i<align_segments.size(); ++i)
    {
        const auto idx = align_segments[i];
        if (idx==-1) break;
        const auto align_type = line[idx].align-1;
        aligns_by_types[align_type] = {idx, (i+1 < 3 && align_segments[i+1] != -1 ? align_segments[i+1] : line.size()) - idx};
    }

    auto x_left = x;
    auto x_right = x + width;

    if (aligns_by_types[0].first != -1) // Left aligh
    {
        x_left = write_line(0, x);
    }

    if (aligns_by_types[2].first != -1) // Right align
    {
        x_right = x + width - size_line(2);
        write_line(2, x_right);
    }

    if (aligns_by_types[1].first != -1) // Center
    {
        auto total = size_line(1);
        auto x_shift = x + ((width - total) >> 1);
        if (x_shift < x_left || x_shift + total > x_right) // We can't fit in space after left/right was placed in - center in left/right gap instead
        {
            x_shift = x + ((x_right-x_left-total) >> 1);
        }
        write_line(1, x_shift);
    }
}

// Assign positions for each slice of text - version when Spacing defined for this line
void TextLine::eval_position_spacing(int x, int y, int width, int height, int default_letter_size)
{
    const auto real_text_width = text_size(default_letter_size).first;
    for(auto& l: line)
    {            
        const auto sz = l.text_size(default_letter_size);
        l.x = x; 
        l.y = y + height - sz.second;
        l.width = sz.first * width / real_text_width;
        x += l.width;
    }
}

void TextLine::check_word_wrap_enabled()
{
    if (align_segments[1] != -1 || align_segments[2] != -1) throw WordWrapError("Can't word wrap line - WordWrap not enabled");
    for(const auto& l: line)
    {
        if (l.word_wrap) return;
    }
    throw WordWrapError("Can't word wrap line - WordWrap not enabled");
}

// Return array of [w, h] for this line.
Size TextLine::text_size(int default_letter_size) const
{
    int w=0;
    int h=16;
    for(const auto& token: line)
    {
        const auto size = token.text_size(default_letter_size);
        w += size.first;
        h = std::max(h, size.second);
    }
    return {w, h};
}

// Performs WordWrap to requested width. Fill array of lines ('target' parameter)
// Raise exception if can't by some reason
void TextLine::word_wrap(std::vector<TextLine> &target, int width, int default_letter_size)
{
    if (text_size(default_letter_size).first <= width) {target.push_back(*this); return;}
    check_word_wrap_enabled();

    std::vector<TextSegment> new_line;
    int rest_width = width;

    auto put = [&](const TextSegment& line) {new_line.push_back(line); rest_width-= line.text_size(default_letter_size).first;};
    Align align = A_None;

    for(auto& l: line)
    {
        if (!l.align) l.align = align;
        align = l.align;
        if (l.text_size(default_letter_size).first <= rest_width) // We can fit this item without splitting
        {
            put(l);
        }
        else // Have to split
        {
            auto spl_item = l.dup(); // We will use it as working item
            bool spl_item_valid = true;
            while(spl_item.text_size(default_letter_size).first > rest_width) // Slice out piece
            {
                auto new_item = spl_item.trim_by_width(rest_width, default_letter_size);

                if (!new_item) // Split is unsuccessfull - flush line and try again
                {
                    if (new_line.empty()) throw WordWrapError("Can't word wrap line - word inside is too long");
                    trim_back(new_line.back().text);
                    target.push_back(clone(new_line)); 
                    new_line.clear();
                    rest_width = width;
                    if (spl_item.text_size(default_letter_size).first <= rest_width)
                    {
                        put(spl_item);
                        spl_item_valid = false;
                        break;
                    }
                    else
                    {
                        new_item = spl_item.trim_by_width(rest_width, default_letter_size);
                        if (!new_item) throw WordWrapError("Can't word wrap line - word inside is too long");
                    }
                }
                put(*new_item);
            }
            if (spl_item_valid && spl_item.text.size()) // Write rest of sliced line
            {
                put(spl_item);
            }
        }
    }
    if (new_line.size()) target.push_back(clone(new_line)); 
}

// Assign positions for each slice of text
void TextLine::eval_position(int x, int y, int width, int default_letter_size)
{
    const auto height = text_size(default_letter_size).second;
    if (align_segments[0] != -1) {eval_position_aligned(x, y, width, height, default_letter_size); return;}
    if (line.empty()) return;
    if (line[0].spacing) {eval_position_spacing(x, y, width, height, default_letter_size); return;}
    for(auto& l: line)
    {
        const auto sz = l.text_size(default_letter_size);
        l.x = x; l.y = y + height - sz.second;
        x += sz.first;
    }
}

void TextLine::add_segment(const TextSegment& text_segment)
{
    line.push_back(text_segment);
    if (!text_segment.align) return;
    int last = -1;
    for(int i=0; i<3; ++i)
    {
        if (align_segments[i] != -1) last=i;
    }
    
    if (last != -1)
    {
        Align prev_ord = line[last].align;
        Align cur_ord = text_segment.align;
        if (cur_ord == prev_ord) throw Error("Duplicated align mark");
        if (cur_ord < prev_ord) throw Error("Align marks placed in wrong order (expected '<', '#', '>')");
    }
    else if (line.size() > 1)
    {
        throw Error("First align mark should be at begining of line");
    }
    align_segments[last+1] = line.size()-1;
}

TextsParserSelected::TextsParserSelected(const TextGlobalDefinition& gd, const TextLine& tl, int width, int height) : obj(gd)
{
    obj.text_lines.push_back(tl);
    if (int w = obj.need_ww(width)) obj = obj.word_wrap(w);
    sz = obj.min_box_size();
    if (sz.first > width || sz.second > height) throw OutOfBounds("Box doesn't fit");
}

void TextsParser::parse_line(const String& line)
{
    static std::regex re("\\\\[cb][^\\\\]*\\\\|\\\\.");

    cur_text_line.reset();
    cur_text_segment.reset(global_definitions);

    for(std::cregex_token_iterator iter(line.data(), line.data() + line.size(), re, {-1, 0}); iter != std::cregex_token_iterator(); ++iter)
    {
        const char* start = iter->first;
        size_t length = iter->length();
        if(start[0] == '\\')
        {
            ++start; --length;
            if (!(length == 1 && start[0] == '\\'))
            {
                if (start[length-1] == '\\') --length;
                cur_text_segment.process_ctrl(String(start, length));
                continue;
            }
        }
        if (length)
        {
            cur_text_segment.text = String(start, length);
            cur_text_line.add_segment(cur_text_segment);
            cur_text_segment.reset();
        }
    }
    text_lines.push_back(cur_text_line);
}

void TextsParser::draw_one_box_centered_imp(LCD& lcd, int x, int y, int width, int height)
{
    const int mw = 2*global_definitions.marging_h;
    const int mh = 2*global_definitions.marging_v;
    auto sz = min_box_size();
    if (sz.second > height - mh || sz.first > width - mw) throw OutOfBounds("Box doesn't fit");
    x += ((width-sz.first-mw) >> 1) + global_definitions.marging_h;
    y += ((height-sz.second-mh) >> 1) + global_definitions.marging_v;
    eval_box(x, y);
    draw_box_to_canvas(lcd, x, y, sz.first, sz.second);
    draw_to_canvas(lcd);
}

void TextsParser::draw_one_box_imp(LCD& lcd, int x, int y, int width, int height)
{
    auto sz = min_box_size();
    if (sz.second > height || sz.first > width) throw OutOfBounds("Box doesn't fit");
    eval_box(x, y, width, height);
    draw_box_to_canvas(lcd, x, y, width, height);
    draw_to_canvas(lcd);
}

int TextsParser::need_ww(int width)
{
    auto sz = min_box_size();
    if (sz.first > width)
    {
        int w = width-reserved_space().first;
        if (w<0) throw OutOfBounds("Not enough space to draw inside Box");
        return w;
    }
    return 0;
}

void TextsParser::parse_text(const String& lines)
{
    static std::regex re("\n");

    text_lines.clear();
    for(std::cregex_token_iterator iter(lines.data(), lines.data() + lines.size(), re, -1); iter != std::cregex_token_iterator(); ++iter)
    {
        parse_line(String(iter->first, iter->second));
    }
    while(!text_lines.empty() && text_lines.back().line.empty()) text_lines.pop_back();
}

// Return array of [w, h] for this block.
Size TextsParser::text_size() const
{
    int w=0;
    int h=0;
    for(const auto &line: text_lines)
    {
        const auto size = line.text_size(global_definitions.letter_size());
        w = std::max(w, size.first);
        h += size.second;
    }
    return {w, h};
}

// Performs word wrap, returns new instance of TextsParser (if wrap is successfull) or raise an exception
TextsParser TextsParser::word_wrap(int width)
{
    std::vector<TextLine> new_text_lines;

    for(auto& it: text_lines)
    {
        it.word_wrap(new_text_lines, width, global_definitions.letter_size());
    }

    TextsParser result(global_definitions);
    result.text_lines = new_text_lines;
    return result;
}

// Assign positions for each line of text
// Margins and Pads not taken in account - caller of this method should take care of it itself by adjusting x,y,width and height
void TextsParser::eval_positions(int x, int y, int width, int height)
{
    const auto sz = text_size();
    if (sz.first > width || sz.second > height) throw OutOfBounds("Text do not fit in bounds");
    auto y_extra = height - sz.second; // Insert this space between rows
    auto y_steps = text_lines.size();
    int row = 0;
    for(auto &line: text_lines)
    {
        if (row)
        {
            const int ins = y_extra / y_steps;
            y += ins;
            y_extra -= ins;
        }
        line.eval_position(x, y, width, global_definitions.letter_size());
        y += line.text_size(global_definitions.letter_size()).second;
        --y_steps;
        ++row;
    }
}

// How much space reserved between Box outside boundary and text inside
Size TextsParser::reserved_space() const {return global_definitions.reserved_space();}

Size TextGlobalDefinition::reserved_space() const
{
    int gap = shadow_width + 2 * std::max<int>(border_width, corner_r ? corner_r + 1 : 0);
    return {2*padding_h + gap, 2*padding_v + gap};
}

// Minimal distance from box top/left to internal text
Size TextGlobalDefinition::min_dist_to_text() const
{
    int gap = std::max<int>(border_width, corner_r ? corner_r + 1 : 0);
    return {padding_h + gap, padding_v + gap};
}

// Evaluate Box size and runs position evaluation
// Returns array with Box definition [<width>, <height>]
Size TextsParser::eval_box(int x, int y, int width, int height)
{
    const auto sz = min_box_size();

    if (!width) width = sz.first;
    if (!height) height = sz.second;

    if (width < sz.first || height < sz.second) throw OutOfBounds("Box doesn't fit");

    const int gap = std::max<int>(global_definitions.border_width, global_definitions.corner_r ? global_definitions.corner_r + 1 : 0);

    const int box_min_gap_left = gap + global_definitions.padding_h;
    const int box_min_gap_right = box_min_gap_left + global_definitions.shadow_width;
    const int box_min_gap_top = gap + global_definitions.padding_v;
    const int box_min_gap_bottom = box_min_gap_top + global_definitions.shadow_width;

    const Size tsz = text_size();

    int dx = box_min_gap_left;
    int dy = box_min_gap_top;
    int w, h;

    if (global_definitions.center_horizontal())
    {
        dx += (width - box_min_gap_right - box_min_gap_left - tsz.first) >> 1;
        w = tsz.first;
    }
    else
    {
        w = width - box_min_gap_left - box_min_gap_right;
    }
    if (global_definitions.center_vertical())
    {
        dy += (height - box_min_gap_bottom - box_min_gap_top - tsz.second) >> 1;
        h = tsz.second;
    }
    else
    {
        h = height - box_min_gap_top - box_min_gap_bottom;
    }

    eval_positions(x+dx, y+dy, w, h);

    return {width, height};
}

void TextsParser::draw_selection_of_boxes(LCD& lcd, const std::vector<int>& sel_array, int x, int y, int width, int height)
{
    std::vector<TextsParserSelected> selected;

    assert(sel_array.size() <= text_lines.size());
    for(const auto idx: sel_array) selected.emplace_back(global_definitions, text_lines[idx], width, height);
    if (selected.size() == 0) return;
    if (selected.size() == 1) {draw_one_box_centered(lcd, x, y, width, height); return;}
    if (global_definitions.equal_size_horizontal() || global_definitions.equal_size_vertical())
    {
        auto sz = selected[0].sz;
        for(const auto &it: selected)
        {
            sz.first = std::max(sz.first, it.sz.first);
            sz.second = std::max(sz.second, it.sz.second);
        }
        for(auto& it: selected) 
        {
            if (global_definitions.equal_size_horizontal()) it.sz.first = sz.first;
            if (global_definitions.equal_size_vertical())   it.sz.second = sz.second;
        }
    }

    int yy = y + 2*global_definitions.marging_v;
    int xx = x + 2*global_definitions.marging_h;
    int total_width = 0;  // Pure width and height of all boxes (without margins)
    int total_height = 0;
    for(auto const& it: selected)
    {
        xx += it.sz.first+global_definitions.marging_h;
        yy += it.sz.second+global_definitions.marging_v;
        total_width += it.sz.first;
        total_height += it.sz.second;
    }

    if (global_definitions.boxes_dir() != 2) // Try vertically first
    {
        if (yy <= height) // We fit vertically
        {
            int total_gap = height - 2*global_definitions.marging_v - total_height;
            int rest_boxes = selected.size() - 1;
            int yy = y + global_definitions.marging_v;
            for(auto& box: selected)
            {
                const int xx = x + ((width - box.sz.first) >> 1);
                box.obj.draw_one_box(lcd, xx, yy, box.sz.first, box.sz.second);
                const int ins = total_gap / rest_boxes;
                yy += box.sz.second + ins;
                --rest_boxes;
                total_gap -= ins;
            }
            return;
        }
    }
    if (global_definitions.boxes_dir() != 1) // Try horizontally last
    {
        if (xx <= width) // We fit horizontally
        {
            int total_gap = width - 2*global_definitions.marging_h - total_width;
            int rest_boxes = selected.size() - 1;
            int xx = x + global_definitions.marging_h;
            for(auto& box: selected)
            {
                const int yy = y + ((height - box.sz.second) >> 1);
                box.obj.draw_one_box(lcd, xx, yy, box.sz.first, box.sz.second);
                const int ins = total_gap / rest_boxes;
                xx += box.sz.first + ins;
                --rest_boxes;
                total_gap -= ins;
            }
            return;
        }
    }
    throw OutOfBounds("Box doesn't fit");
}

} // namespace TextBoxDraw
