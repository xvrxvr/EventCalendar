#include "common.h"
#include "box_draw.h"
#include "hadrware.h"


void RectList::sub(const Rect& clip)
{
    int limit = filled;
    for(int idx=0; idx < limit; ++idx)
    {
        Rect& r = rects[idx];
        int x1=r.x, y1=r.y, x2=r.x+r.w, y2 = r.y+r.h;
        int xx1=clip.x, yy1=clip.y, xx2=clip.x+clip.w, yy2 = clip.y+clip.h;

        if (x1 >= xx2 or y1 >= yy2 or x2 <= xx1 or y2 <= yy1) continue;

        xx1 = std::min(x2, std::max(x1, xx1));
        xx2 = std::min(x2, std::max(x1, xx2));
        yy1 = std::min(y2, std::max(y1, yy1));
        yy2 = std::min(y2, std::max(y1, yy2));
        /*
         +---+---------+---+  < 8
         | 9 |         | A |
         +---+         +---+
         |                 |
         |                 |
         +---+         +---+
         | 5 |         | 6 |
         +---+---------+---+  < 4

         ^                 ^
         1                 2

        */
        int fit = (x1 == xx1 ? 1 : 0) | (x2 == xx2 ? 2 : 0) | (y1 == yy1 ? 4 : 0) | (y2 == yy2 ? 8 : 0);
        assert(fit !=0);
        switch(fit)
        {
            case 15: continue;
            case 15-1: /*cut right part*/  x2 = xx1; break;
            case 15-2: /*cur left part*/   x1 = xx2; break;
            case 15-8: /*cut bottom part*/ y1 = yy2; break;
            case 15-4: /*cur top part*/    y2 = yy1; break;
            case 5: /*cut left bottom - split*/     put(Rect(xx2, y1, x2 -  xx2, yy2 - y1, r.color));  y1 = yy2; break;
            case 6: /*cut right bottom - split*/    put(Rect(x1,  y1, x1 -  xx1, yy2 - y1, r.color));  y1 = yy2; break;
            case 9: /*cur top left - split*/        put(Rect(xx2, yy1, x2 - xx2, y2 -  yy1, r.color)); y2 = yy1; break;
            case 10: /*cur top right - split*/      put(Rect(x1,  yy1, x1 - xx1, y2 -  yy1, r.color)); y2 = yy1; break;
            default: assert(false && "Unsupported fit value"); break;
        }
        r.x=x1; r.y = y1;
        r.w=x2-x1; r.h = y2-y1;
    }
}


void PicBox::put_line(int x, int y, int length, Color color, bool clip)
{
    if (length < 0) {x += length + 1; length = -length;}
    if (clip && (y< 0 || y>= h)) return;
    assert(y>= 0);
    uint8_t* line = img.get() + w*(h - y - 1);
    for(int idx = 0; idx < length; ++idx, ++x)
    {
        if (clip && (x<0 || x >= w)) continue;
        assert(x >= 0);
        line[x] = color;
    }
}

void PicBox::put_box(int x, int y, int w, int h, Color color, bool clip)
{
    if (h < 0) {y += h + 1; h = -h;}
    for(int idx=0; idx<h; ++idx, ++y)
    {
        if (!clip || !(y < 0 || y >= h)) put_line(x, y, w, color, clip);
    }
}

void PicBox::draw_corner_box(char type, int corner, int x, int y, int r, Color color, int bwidth)
{
    if (type == '-')  return;
    int w = (corner & 1) == 0 ? x : this->w - x;
    int h = (corner & 2) ? y : this->h - y;
    char bt = 0;
    switch(type)
    {
        case 'H': h = r; if ((corner & 2) == 0) {h ++; bt = 'T';} else bt = 'B'; break;
        case 'V': w = r; if (corner & 1) {w ++; bt = 'R';} else bt = 'L'; break;
        default: return;
    }
    if (corner & 2) {h = -h;  y--;}
    if ((corner & 1) == 0) {w = -w; x--;}
    if (!w || !h) return;
    if (h < 0) {y += h + 1; h = -h;}
    if (w < 0) {x += w + 1; w = -w;}
    put_box(x, y, w, h, color);
    if (!bwidth) return;
    switch(bt)
    {
        case 'R': put_box(x + w - bwidth, y, bwidth, h, C_Border); break;
        case 'L': put_box(x, y, bwidth, h, C_Border); break;
        case 'T': put_box(x, y+ h - bwidth, w, bwidth, C_Border); break;
        case 'B': put_box(x, y, w, bwidth, C_Border); break;
        default: break;
    }
}

void PicBox::draw_arc(int corner, int x, int y, int r, Color color, int bwidth)
{
    BresLine arc(r); arc.run();
    int delta_y = corner & 2 ? -1 : 1;
    int delta_x = corner & 1 ? 1 : -1;
    int yy = y;
    for(auto bx: arc)
    {
        put_line(x, yy, bx*delta_x, color);
        yy += delta_y;
    }
    if (bwidth)
    {
        BresPoints brd(r); brd.run();
        int dx = -bwidth*delta_x;
        int dy = -bwidth*delta_y;
        for(auto [arc_x, arc_y]: brd)
        {
            put_box(x + arc_x*delta_x, y + arc_y*delta_y, dx, dy, C_Border, true);
        }
    }
    draw_corner_box("-HVF"[corner], 0, x, y, r, color, bwidth);
    draw_corner_box("H-FV"[corner], 1, x, y, r, color, bwidth);
    draw_corner_box("VF-H"[corner], 2, x, y, r, color, bwidth);
    draw_corner_box("FVH-"[corner], 3, x, y, r, color, bwidth);
}
        

void BoxCreator::draw_pic_box(PicBox& pic, int corner)
{
    int h = pic.get_h()-1;
    int x = corner & 1 ? 0 : r;
    int y = corner & 2 ? h : h-r;
    if (s and corner != 0) pic.draw_arc(corner, x+s, y-s, r, C_Shadow);
    pic.draw_arc(corner, x, y, r, C_Body, b);
}

BoxCreator::BoxCreator(int width, int height, int r, int border_width, int shadow_width) 
    : result(expect_rects(r, border_width, shadow_width)), 
      r(r), s(shadow_width), b(border_width), total_height(height+shadow_width)
{
    /*
    Creates Box with border, rounded corners and shadow
    Internally all coordinates are relatives to box bottom/left corner. If shadow requested its Y coordinate is negative.
    On BoxDef emit they recalculated and flipped

    Full layout of box is follow:

    +---+============+---+  or +---+============+---+
    | 0 |            | 1 |     |   |            |   |
    +---+            +---+     +---+            +---+
    I                   IS     I                    I
    I                   IS     I                    I
    +---+            +---+     +---+            +---+
 0> | 2 |============| 3 |     |   |            |   |
    +---+SSSSSSSSSSSS+---+     +---+============+---+  < 0
                        ^ - width points here
    */
    result.put(Rect(0, 0, width, height, C_Body));

    if (border_width)
    {
        add_rect_list(Rect(0, 0, width, border_width, C_Border));
        add_rect_list(Rect(0, height-border_width, width, border_width, C_Border));
        add_rect_list(Rect(0, 0, border_width, height, C_Border));
        add_rect_list(Rect(width-border_width, 0, border_width, height, C_Border));
    }

    if (shadow_width)
    {
        add_rect_list(Rect(shadow_width, -shadow_width, width, shadow_width, C_Shadow));
        add_rect_list(Rect(width, -shadow_width, shadow_width, height, C_Shadow));
    }

    if (r)
    {
        int box_size = r + 1 + shadow_width;
        for(int bidx=0; bidx<4; ++bidx)
        {
            PicBox pic(box_size, box_size);
            draw_pic_box(pic, bidx);
            int x = bidx & 1 ? width + shadow_width - box_size : 0;
            int y = bidx & 2 ? -shadow_width : height - box_size;
            images[bidx].reset(pic.detach_img());
            add_rect_list(Rect(x, y, box_size, box_size, Color(C_Img0 + bidx)));
        }
    }
}

void BoxCreator::draw(LCD& lcd, int dx, int dy, uint16_t* pallete, bool with_clip)
{
    for(const auto& box: result)
    {
        int box_y = total_height - s - box.y - box.h;
        if (box.color < C_Img0)
        {
            lcd.WRect(box.x+dx, box_y+dy, box.w, box.h, pallete[box.color]);
        }
        else
        {
            lcd.WImgPallete(box.x+dx, box_y+dy, box.w, box.h, images[box.color-C_Img0], pallete, with_clip);
        }
    }
}
