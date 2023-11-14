"use strict";

class WordWrapError extends Error {}
class OutOfBounds extends Error {}

class TextGlobalDefinition {
    #model2html_update_list = []

    marging_v = 5;  // ???
    marging_h = 5;
    padding_v = 10;
    padding_h = 10;
    border_width = 0;
    border_color = 0;
    shadow_width = 0;
    shadow_color = color_to_rgb565('999999');
    corner_r = 0;
    fg_color = '';  // FG color (rgb565 hex string)
    bg_color = '';  // BG color (rgb565 hex string)
    letter_size = 0;    // 0 - auto, 1,2 - defined size
    word_wrap = false;  // Enable word wrap (by spaces)
    boxes_dir = 0;      // Multiple boxes direction: 0 - auto, 1 - boxes placed vertically, 2 - boxes placed horizontaly
    keyb_type = '';     // Keyboard type: [e][r][d][c]
    fuzzy_dist = '30%';    // Text distance for Fuzzy compare. Number (in symbols) or string in form <???>% - for percent of answer length (excluding digits)
    min_age = 10;
    max_age = 99;

    connect_to_html_form()
    {
        // Number (input type=number):
        for(const id of ["marging_v", "marging_h", "padding_v", "padding_h", "border_width", "shadow_width", "corner_r"])
        {
            const html_item = document.getElementById(`hdr_${id}`);
            this.#model2html_update_list.push( () => {html_item.value = this[id];});
            html_item.onchange = () => {this[id] = html_item.value - 0; this.update_from_html();};
        }
        // Text (input type="text"):
        {
            const html_item = document.getElementById('hdr_fuzzy_dist');
            this.#model2html_update_list.push( () => {html_item.value = this.fuzzy_dist;});
            html_item.onchange = () => {this.fuzzy_dist = html_item.value; this.update_from_html();};
        }
        // Word Wrap - checkbox
        {
            const html_ww_item = document.getElementById('hdr_word_wrap');
            this.#model2html_update_list.push( () => {html_ww_item.checked  = this.word_wrap;});
            html_ww_item.onchange = () => {this.word_wrap = html_ww_item.checked; this.update_from_html();};
        }
        // Color (input type=color):
        for(const id of ["border_color", "shadow_color", "fg_color", "bg_color"])
        {
            const html_item = document.getElementById(`hdr_${id}`);
            this.#model2html_update_list.push( () => {html_item.value = `#${rgb565_to_color(this[id])}`;});
            html_item.onchange = () => {
                this[id] = color_to_rgb565(html_item.value.slice(1)); 
                html_item.value = `#${rgb565_to_color(this[id])}`;
                this.update_from_html();
            };
        }
        // Selection (selection)
        for(const id of ["boxes_dir", "letter_size"])
        {
            const html_item = document.getElementById(`hdr_${id}`);
            this.#model2html_update_list.push( () => {html_item.selectedIndex = this[id];});
            html_item.onchange = () => {this[id] = html_item.selectedIndex; this.update_from_html();};
        }
        // keyb_type Selection Muilti
        {
            const html_item = document.getElementById('hdr_keyb_type');
            this.#model2html_update_list.push( () => {
                for(const opt of html_item.options)
                {
                    opt.selected = this.keyb_type.includes(opt.value);
                }
            });
            html_item.onchange = () => {
                let acc = '';
                for(const opt of html_item.selectedOptions) acc += opt.value;
                this.keyb_type = acc;
                this.update_from_html();
           };
        }
    }

    update_to_html()
    {
        for(let f of this.#model2html_update_list) f();
    }

    update_from_html = () => {};

    get header()
    {
        let result = base64_digit(this.marging_v) + base64_digit(this.marging_h) + base64_digit(this.padding_v) + base64_digit(this.padding_h) + base64_digit(this.border_width);
        result += base64_digit(this.shadow_width) + base64_digit(this.corner_r) + base64_digit(this.fuzzy_dist.replace(/%$/, "") - 0);
        result += hex(this.min_age) + hex(this.max_age);

        let kb_sel = 0;
        for(const s of this.keyb_type)
        {
            kb_sel |= {'e': 1, 'r': 2, 'n': 4, 'c': 8}[s];
        }
        result += hex(kb_sel, 1);

        let opt = this.boxes_dir | (this.letter_size<<2);
        if (this.word_wrap) opt |= 0x10;
        if (this.fuzzy_dist.endsWith('%')) opt |= 0x20;
        result += hex(opt) + ' ';

        const c = (color, comma=true) => {
            result += color ? color : '-'; 
            if (comma) result += ',';
        };

        c(this.bg_color); c(this.border_color); c(this.shadow_color); c(this.fg_color, false);

        return result;
    }

    set header(value)
    {
        const vals = value.split(/\s+/);

        this.marging_v    = unbase64_digit(value[0]);
        this.marging_h    = unbase64_digit(value[1]);
        this.padding_v    = unbase64_digit(value[2]);
        this.padding_h    = unbase64_digit(value[3]);
        this.border_width = unbase64_digit(value[4]);
        this.shadow_width = unbase64_digit(value[5]);
        this.corner_r     = unbase64_digit(value[6]);
        this.fuzzy_dist   = unbase64_digit(value[7]);
        this.min_age      = Number.parseInt(value.slice(8, 10), 16);
        this.max_age      = Number.parseInt(value.slice(10, 12), 16);

        let kb_sel = Number.parseInt(value.slice(12, 13), 16);
        this.keyb_type = '';
        if (kb_sel & 1) this.keyb_type += 'e';
        if (kb_sel & 2) this.keyb_type += 'r';
        if (kb_sel & 4) this.keyb_type += 'n';
        if (kb_sel & 8) this.keyb_type += 'c';

        let opt = Number.parseInt(value.slice(12, 13), 16);
        this.boxes_dir = opts & 3;
        this.letter_size = (opt >> 2) & 3;
        this.word_wrap = (opt & 0x10)!=0;
        if (opt & 0x20) this.fuzzy_dist += '%';

        const colors      = vals[1].split(',');
        this.bg_color     = colors[0] == '-' ? '' : colors[0];
        this.border_color = colors[1] == '-' ? '' : colors[1];
        this.shadow_color = colors[2] == '-' ? '' : colors[2];
        this.fg_color     = colors[3] == '-' ? '' : colors[3];       
    }


    // Process '\...' control code. 'symbol' is a pure code (without '\')
    process_ctrl(symbol) 
    {
        let spl = symbol.split(':')
        switch(spl[0])
        {
            case 'm':  this.marging_v = this.marging_h = spl[1] - 0; break;
            case 'mv': this.marging_v = spl[1] - 0; break;
            case 'mh': this.marging_h = spl[1] - 0; break;
            case 'p':  this.padding_v = this.padding_h = spl[1] - 0; break;
            case 'pv': this.padding_v = spl[1] - 0; break;
            case 'ph': this.padding_h = spl[1] - 0; break;
            case 'b':  this.border_width = spl[1] - 0; if (spl.length > 2) this.border_color = spl[2]; break;
            case 's':  this.shadow_width = spl[1] - 0; if (spl.length > 2) this.shadow_color = spl[2]; break;
            case 'r':  this.corner_r = spl[1] - 0; break;
            case 'bg': this.bg_color = check_color_name(spl[1]); break;
            case 'fg': this.fg_color = check_color_name(spl[1]); break;
            case '1':  this.letter_size = 1; break;
            case '2':  this.letter_size = 2; break;
            case 'W':  this.word_wrap = true; break;
            case 'd':  this.boxes_dir = spl[1]; if (!(/^(d|v)$/.test(this.boxes_dir))) throw Error(`Global definition 'd:<v|h>' expect 'v' or 'h' symbols, but found '${spl[1]}'`); break;
            case 'st': this.keyb_type = spl[1]; if (!(/^([edr]+|c)$/.test(this.keyb_type))) throw Error(`st definition wrong '${this.keyb_type}'. Expected [e][r][d] or c`); break;
            case 'dist': this.fuzzy_dist = spl[1]; if (!(/^\\d+%?$/.test(this.fuzzy_dist))) throw Error(`dist definition wrong '${this.fuzzy_dist}'. Expected number or percent`); break;
            default: throw Error(`Unknown Global definition '${symbol}'`);
        }
    }

    new_text_segment()
    {
        let tseg = new TextSegment();
        tseg.letter_size = this.letter_size;
        tseg.spacing     = this.spacing;
        tseg.fg_color    = this.fg_color;
        tseg.bg_color    = this.bg_color;
        tseg.word_wrap   = this.word_wrap;
        return tseg;
    }

    toString()
    {
        let result = [];
        if (this.marging_v) result.push(`marging_v: ${this.marging_v}`);
        if (this.marging_h) result.push(`marging_h: ${this.marging_h}`);
        if (this.padding_v) result.push(`padding_v: ${this.padding_v}`);
        if (this.padding_h) result.push(`padding_h: ${this.padding_h}`);
        if (this.border_width) result.push(`border: ${this.border_width}:${this.border_color}`);
        if (this.shadow_width) result.push(`shadow: ${this.shadow_width}:${this.shadow_color}`);
        if (this.corner_r) result.push(`corner: ${this.corner_r}`);
        if (this.boxes_dir) result.push(`boxdir: ${this.boxes_dir}`);
        return result.join(', ');
    }
}

function base64_digit(val)
{
    if (val < 10) return String.fromCharCode(val + "0".charCodeAt(0));
    val -= 10;
    if (val < 26) return String.fromCharCode(val + "A".charCodeAt(0));
    val -= 26;
    if (val < 26) return String.fromCharCode(val + "a".charCodeAt(0));
    val -= 26;
    switch(val)
    {
        case 0: return '+';
        case 1: return '-';
        default: return '?';
    }
}

function hex(val, digits=2)
{
    return val.toString(16).toUpperCase().padStart(digits, '0');
}

function unbase64_digit(val)
{
    const code = val.charCodeAt(0);
    if (val >= '0' && val <= '9') return code - "0".charCodeAt(0);
    if (val >= 'A' && val <= 'Z') return code - "A".charCodeAt(0) + 10;
    if (val >= 'a' && val <= 'z') return code - "a".charCodeAt(0) + 36;
    switch(val)
    {
        case '+': return 62;
        case '-': return 63;
        default: return 0;
    }
}

function rgb565_to_color(rgb565)
{
    if (typeof rgb565 == 'string') rgb565 = Number.parseInt(rgb565, 16);
    let r = (rgb565 >> 11) & 0x1F;
    let g = (rgb565 >> 5) & 0x3F;
    let b = rgb565 & 0x1F;

    if (r & 1) {r <<= 3; r |= 7;} else r <<= 3;
    if (g & 1) {g <<= 2; g |= 3;} else g <<= 2;
    if (b & 1) {b <<= 3; b |= 7;} else b <<= 3;

    return hex(r) + hex(g) + hex(b);
}


function color_to_rgb565(color)
{
    const r = Number.parseInt(color.slice(0, 2), 16) >> 3;
    const g = Number.parseInt(color.slice(2, 4), 16) >> 2;
    const b = Number.parseInt(color.slice(4, 6), 16) >> 3;
    return hex((r << 11) | (g << 5) | b, 4);
}



class TextSegment {
    #letter_size(default_letter_size, total_letters)
    {
        if (this.letter_size) default_letter_size = this.letter_size;
        if (!default_letter_size) default_letter_size = 1;
        return [total_letters * (default_letter_size == 2 ? 16 : 8), default_letter_size == 2 ? 32 : 16];
    }

    letter_size = 0;    // 0 -auto, 1,2 - defined size
    align = '';         // '' - no align, else < or > or #
    spacing = false;    // false - no spacing, true - spacing at each symbol
    fg_color = '';      // FG color (hex string)
    bg_color = '';      // BG color (hex string)
    word_wrap = false;  // Enable word wrap (by spaces)
    text = '';          // Line of text
    x = 0;        // Coordinates for drawing
    y = 0;
    width = 0;    // Optional width to fit (in case of space insertion mode)

    get align_ord()
    {
        return {'': 0, '<': 1, '#': 2, '>': 3} [this.align];
    }

    get have_letter_size() {return this.letter_size != 0;}

    // Return array of [w, h] for this segment.
    text_size(default_letter_size=0)
    {
        return this.#letter_size(default_letter_size, this.text.length);
    }

    // Process '\...' control code. 'symbol' is a pure code (without '\')
    process_ctrl(symbol) 
    {
        switch(symbol.at(0))
        {
            case '1': this.letter_size = 1; break;
            case '2': this.letter_size = 2; break;
            case '<': case '>': case '#': this.align = symbol.at(0); break;
            case 's': this.spacing = true; break;
            case 'W': this.word_wrap = true; break;
            case 'c': this.fg_color = symbol.slice(1); break;
            case 'b': this.bg_color = symbol.slice(1); break;
            default: this.text += symbol; break;
        }
    }

    // Clone all control from current to new one, except Align and text itself
    clone(text = '')
    {
        let result = new TextSegment();
        result.letter_size = this.letter_size;
        result.spacing     = this.spacing;
        result.fg_color    = this.fg_color;
        result.bg_color    = this.bg_color;
        result.word_wrap   = this.word_wrap;
        result.text = text;
        return result;
    }

    // Create full duplicate
    dup(text = null)
    {
        let result = this.clone(text ?? this.text);
        result.align = this.align;
        return result;
    }

    // Split this item in 2 (if possible) on required 'width'. Returns new item (with limited width) and leave rest of string in this one.
    // Returns new item or 'undefined' if can't split
    trim_by_width(width, default_letter_size=0)
    {
        const let_width = this.#letter_size(default_letter_size, 1)[0];
        let part = '';
        const splited = this.text.split(/\s+/);
        for(let idx=0; idx<splited.length; ++idx)
        {
            const s = splited[idx];
            if (let_width*(part.length + s.length) > width) // Split here
            {
                if (!idx) return;
                this.text = splited.slice(idx).join(' ').trimStart();
                return this.dup(part.trimEnd());
            }
            if (idx) part += ' ';
            part += s;
        }
    }

    // JS only method to draw current object to Canvas
    draw_to_canvas(canvas, last_in_line, default_letter_size=0)
    {
        //l.draw_to_canvas(canvas);
        const sz = this.#letter_size(default_letter_size, 1);
        canvas.font = `${sz[1]}px monospace`;
        let x = this.x;
        if (this.bg_color) 
        {
            canvas.fillStyle = `#${this.bg_color}`;
            canvas.fillRect(this.x, this.y, sz[0]*this.text.length, sz[1]);
        }
        if (this.fg_color) canvas.fillStyle = `#${this.fg_color}`; else canvas.fillStyle = '#000';

        let incremental;

        if (this.spacing && this.width)
        {
            const target_width = this.width;
            const my_width = sz[0] * this.text.length;
            const my_increment = sz[0];
            let current_width = 0;
            incremental = () => {
                current_width += my_increment;
                x = this.x + Math.floor(target_width * current_width / my_width);
            };
        }
        else
        {
            incremental = () => {x += sz[0];};
        }

        for(let sym of this.text)
        {
            canvas.fillText(sym, x, this.y+sz[1], sz[1]);
            incremental();
        }
    }

    toString()
    {
        let result = '';
        if (this.letter_size) result += `[Size: ${this.letter_size}] `;
        if (this.align)       result += `[Align: ${this.align}] `;
        if (this.spacing)     result += `[Spacing] `;
        if (this.fg_color)    result += `[FG: ${this.fg_color}] `;
        if (this.bg_color)    result += `[BG: ${this.bg_color}] `;
        if (this.word_wrap)   result += '[WordWrap] ';
        return result + `(${this.x}x${this.y}:${this.width})  "${this.text}"`;
    }
}


class TextLine {
    // Assign positions for each slice of text - version when Align defined for this line
    #eval_position_aligned(x, y, width, height, default_letter_size)
    {
        let aligns = this.align_segments.flat(); // Copy array
        let aligns_by_types = [null, null, null]; // Aligns rearranged by types. Value - null (no segmnent) or [<start index>, <length>] (for segment)

        const write_line = (al_type, xx) => {
            for(let i = 0; i < aligns_by_types[al_type][1]; ++i)
            {
               let l = this.line[i+aligns_by_types[al_type][0]];
               const sz = l.text_size(default_letter_size);
               l.x = xx; l.y = y + height - sz[1];
               xx += sz[0];
            }
            return xx;
        };

        const size_line = (al_type) => {
            let total = 0;
            for(let i = 0; i < aligns_by_types[al_type][1]; ++i) 
                total += this.line[i+aligns_by_types[al_type][0]].text_size(default_letter_size)[0];
            return total;
        };

        aligns.push(this.line.length);
        for(let i=0; i<this.align_segments.length; ++i)
        {
            const align_type = this.line[this.align_segments[i]].align_ord-1;
            aligns_by_types[align_type] = [this.align_segments[i], aligns[i+1] - aligns[i]];
        }

        let x_left = x;
        let x_right = width;

        if (aligns_by_types[0]) // Left aligh
        {
            x_left = write_line(0, x);
        }

        if (aligns_by_types[2]) // Right align
        {
            x_right = x + width - size_line(2);
            write_line(2, x_right);
        }

        if (aligns_by_types[1]) // Center
        {
            let total = size_line(1);
            let x_shift = x + ((width - total) >> 1);
            if (x_shift < x_left || x_shift + total > x_right) // We can't fit in space after left/right was placed in - center in left/right gap instead
            {
                x_shift = x + ((x_right-x_left-total) >> 1);
            }
            write_line(1, x_shift);
        }
    }

    // Assign positions for each slice of text - version when Spacing defined for this line
    #eval_position_spacing(x, y, width, height, default_letter_size)
    {
        const real_text_width = this.text_size(default_letter_size)[0];
        for(let l of this.line)
        {            
            const sz = l.text_size(default_letter_size);
            l.x = x; 
            l.y = y + height - sz[1];
            l.width = Math.floor(sz[0] * width / real_text_width);
            x += l.width;
        }
    }

    line = [];
    align_segments = [];

    get raw_text() {return this.line.reduce( (acc, cur_value) => acc + cur_value.text, '');}

    get have_letter_size() {return this.line.some( (element) => element.have_letter_size);}

    // Return array of [w, h] for this line.
    text_size(default_letter_size=0)
    {
        let w=0;
        let h=0;
        for(let token of this.line)
        {
            const size = token.text_size(default_letter_size);
            w += size[0];
            h = Math.max(h, size[1]);
        }
        return [w, h];
    }

    clone(new_line)
    {
        result = new TextLine();
        result.line = new_line;
        result.align_segments = this.align_segments;
        return result;
    }

    // Performs WordWrap to requested width. Fill array of lines ('target' parameter)
    // Raise exception if can't by some reason
    word_wrap(target, width, default_letter_size=0)
    {
        if (this.text_size(default_letter_size)[0] <= width) {target.push(this); return;}
        if (this.align_segments.length > 1 || !this.line.some( (element) => element.word_wrap)) throw new WordWrapError(`Can't word wrap line "${this.raw_text}" - WordWrap not enabled`);

        let new_line = [];
        let rest_width = width;

        const put = (line) => {new_line.push(line); rest_width-= line.text_size(default_letter_size)[0];};
        let align = null;

        for(let l of this.line)
        {
            if (!l.align) l.align = align;
            align = l.align;
            if (l.text_size(default_letter_size)[0] <= rest_width) // We can fit this item without splitting
            {
                put(l);
            }
            else // Have to split
            {
                let spl_item = l.dup(); // We will use it as working item
                while(spl_item.text_size(default_letter_size)[0] > rest_width) // Slice out piece
                {
                    let new_item = spl_item.trim_by_width(rest_width, default_letter_size);

                    if (!new_item) // Split is unsuccessfull - flush line and try again
                    {
                        if (!new_line.length) throw new WordWrapError(`Can't word wrap line "${this.raw_text}" - word inside is too long`);
                        new_line.at(-1).text = new_line.at(-1).text.trimEnd();
                        target.push(this.clone(new_line)); 
                        new_line = [];
                        rest_width = width;
                        if (spl_item.text_size(default_letter_size)[0] <= rest_width)
                        {
                            put(spl_item);
                            spl_item = null;
                            break;
                        }
                        else
                        {
                            new_item = spl_item.trim_by_width(rest_width, default_letter_size);
                            if (!new_item) throw new WordWrapError(`Can't word wrap line "${this.raw_text}" - word inside is too long`);
                        }
                    }
                    put(new_item);
                }
                if (spl_item?.text.length) // Write rest of sliced line
                {
                    put(spl_item);
                }
            }
        }
        if (new_line.length) target.push(this.clone(new_line)); 
    }

    // Assign positions for each slice of text
    eval_position(x, y, width, default_letter_size=0)
    {
        const height = this.text_size(default_letter_size)[1];
        if (this.align_segments.length) {this.#eval_position_aligned(x, y, width, height, default_letter_size); return;}
        if (!this.line.length) return;
        if (this.line[0].spacing) {this.#eval_position_spacing(x, y, width, height, default_letter_size); return;}
        for(let l of this.line)
        {
            const sz = l.text_size(default_letter_size);
            l.x = x; l.y = y + height - sz[1];
            x += sz[0];
        }
    }

    add_segment(text_segment)
    {
        this.line.push(text_segment);
        if (!text_segment.align) return;
        if (this.align_segments.length)
        {
            let prev_ord = this.line[this.align_segments.at(-1)].align_ord;
            let cur_ord = text_segment.align_ord;
            if (cur_ord == prev_ord) throw Error("Duplicated align mark");
            if (cur_ord < prev_ord) throw Error("Align marks placed in wrong order (expected '<', '#', '>')");

        }
        else if (this.line.length > 1)
        {
            throw Error("First align mark should be at begining of line");
        }
        this.align_segments.push(this.line.length-1);
    }

    // JS only method to draw current object to Canvas
    draw_to_canvas(canvas, default_letter_size=0)
    {
        for(let i = 0; i < this.line.length; ++i) 
        {
            this.line[i].draw_to_canvas(canvas, i+1 == this.line.length, default_letter_size);
        }
    }

    toString()
    {
        let result = '';
        if (this.align_segments) result += `  AlignSegments: ${this.align_segments}\n`;
        for(let ll of this.line)
        {
            result += `   ${ll}\n`;
        }
        return result;
    }
}

// Check color name and syntax. Raise error if not color
function check_color_name(color)
{
    return color;
}

class TextsParser {
    #prev_in_line_was_text = false;
    #cur_text_line;

    #parse_global_line(line)
    {
        for(let ctrl of line.split(/\s+/))
        {
            if (ctrl) this.global_definitions.process_ctrl(ctrl);
        }
    }

    #flush_text()
    {
        if (this.#prev_in_line_was_text)
        {
            if (this.#cur_text_segment.text) this.#cur_text_line.add_segment(this.#cur_text_segment);
            this.#cur_text_segment = this.#cur_text_segment.clone();
            this.#prev_in_line_was_text = false;
        }
    }

    #cur_text_segment = new TextSegment();

    #parse_line(line)
    {
        this.#cur_text_line = new TextLine();
        this.#prev_in_line_was_text = false;
        this.#cur_text_segment = this.global_definitions.new_text_segment();
        for(let str of line.split(/(\\[cb][^\\]*\\|\\.)/))
        {
            if(str.startsWith('\\'))
            {
                this.#flush_text();
                str = str.slice(1);
                if (str.endsWith('\\')) str = str.slice(0, -1);
                this.#cur_text_segment.process_ctrl(str);
            }
            else
            {
                this.#cur_text_segment.text += str;
                this.#prev_in_line_was_text = true;
            }
        }
        this.#flush_text();
        this.text_lines.push(this.#cur_text_line);
    }

/////////////////////// Public part ///////////////////////////////////////////////
    global_definitions = new TextGlobalDefinition();
    text_lines = [];

    parse_text(lines)
    {
        this.text_lines = [];
        for(let str of lines.split('\n'))
        {
            if (str.startsWith('\\!')) this.#parse_global_line(str.slice(2));
            else this.#parse_line(str);
        }
        while(this.text_lines.length && !this.text_lines.at(-1).line.length) this.text_lines.pop();
    }

    get have_letter_size() {return this.text_lines.some( (element) => element.have_letter_size);}

    // Return array of [w, h] for this block.
    text_size()
    {
        let w=0;
        let h=0;
        for(let line of this.text_lines)
        {
            const size = line.text_size(this.global_definitions.letter_size);
            w = Math.max(w, size[0]);
            h += size[1];
        }
        return [w, h];
    }

    // Performs word wrap, returns new instance of TextsParser (if wrap is successfull) or raise an exception
    word_wrap(width)
    {
        let new_text_lines = [];

        for(let it of this.text_lines)
        {
            it.word_wrap(new_text_lines, width, this.global_definitions.letter_size);
        }

        let result = new TextsParser();
        result.global_definitions = this.global_definitions;
        result.text_lines = new_text_lines;
        return result;
    }

    // Assign positions for each line of text
    // Margins and Pads not taken in account - caller of this method should take care of it itself by adjusting x,y,width and height
    eval_positions(x, y, width, height)
    {
        const sz = this.text_size();
        if (sz[0] > width || sz[1] > height) throw new OutOfBounds(`Text do not fit in bounds: text WxH is ${sz[0]}x${sz[1]}, but place only ${width}x${height}`);
        let y_extra = height - sz[1]; // Insert this space between rows
        let y_steps = this.text_lines.length;
        let row = 0;
        for(let line of this.text_lines)
        {
            if (row)
            {
                const ins = Math.floor(y_extra / y_steps);
                y += ins;
                y_extra -= ins;
            }
            line.eval_position(x, y, width, this.global_definitions.letter_size)
            y += line.text_size(this.global_definitions.letter_size)[1];
            --y_steps;
            ++row;
        }
    }

    // Evaluate Box size and runs position evaluation
    // Returns array with Box definition [<width>, <height>]
    eval_box(x, y)
    {
        const sz = this.text_size();
        const inner_width = sz[0] + 2*this.global_definitions.padding_h;
        const inner_height = sz[1] + 2*this.global_definitions.padding_v;

        x += this.global_definitions.padding_h + this.global_definitions.border_width;
        y += this.global_definitions.padding_v + this.global_definitions.border_width;

        this.eval_positions(x, y, sz[0], sz[1]);

        return [inner_width + 2*this.global_definitions.border_width + this.global_definitions.shadow_width, 
            inner_height + 2*this.global_definitions.border_width + this.global_definitions.shadow_width];
    }

    // JS only method to draw current TextParser to Canvas
    draw_to_canvas(canvas)
    {
        for(let line of this.text_lines) line.draw_to_canvas(canvas, this.global_definitions.letter_size);
    }

    toString()
    {
        let result = `Globals: ${this.global_definitions}\nTotal ${this.text_lines.length} lines of text:\n`;
        let idx = 0;
        for(let l of this.text_lines)
        {
            result += `\nLine ${idx++}:\n${l}`;
        }
        return result;
    }
}
