#pragma once

class BresenhameBase {
protected:
    int r;
public:
    BresenhameBase(int r) : r(r) {}

    void run()
    {
        int x = 0;
        int y = r;
        put_pnt(x, y);
        int p = 3-2*r;
        while(y>x)
        {
            if (p < 0) {p += 4*x+6; ++x;}
            else {p += 4*(x-y)+10; ++x, --y;}
            put_pnt(x, y);
        }
    }

    virtual void put_pnt(int x, int y) = 0;
};


/*
    Generates upper right sector of circle

    r .
    ..
    ...
.........
    0 .....
*/
class BresLine : public BresenhameBase {
    std::unique_ptr<uint8_t[]> result;

public:
    BresLine(int r) : BresenhameBase(r), result(new uint8_t[r+1])
    {
        memset(result.get(), -1, r+1);
    }

    virtual void put_pnt(int x, int y) override
    {
        assert(y < r+1);
        assert(x < r+1);
        result[y] = x;
        if (result[x] == 0xFF) result[x] = y;
    }

    uint8_t operator[] (int idx) {return result[idx];}

    uint8_t* begin() {return result.get();}
    uint8_t* end() {return result.get() + r + 1;}

    size_t size() const {return r+1;}
};


/*
    Generates series of points (x, y)
*/
class BresPoints : public BresenhameBase {
    std::unique_ptr<std::pair<uint8_t, uint8_t>[]> result;
    size_t filled = 0;
public:
    using XY = std::pair<uint8_t, uint8_t>;

    BresPoints(int r) : BresenhameBase(r), result(new XY[2*(r+1)]) {}

    virtual void put_pnt(int x, int y)
    {
        assert(filled + 2 <= 2*(r+1));
        result[filled++] = XY{x, y};
        result[filled++] = XY{y, x};
    }

    const XY& operator[] (int idx) {return result[idx];}

    XY* begin() {return result.get();}
    XY* end() {return result.get() + filled;}

    size_t size() const {return filled;}
};


enum Color {
    C_None      = 0,
    C_Body      = 1,
    C_Border    = 2,
    C_Shadow    = 3,
    C_Img0      = 4,
    C_Img1      = 5,
    C_Img2      = 6,
    C_Img3      = 7
};

struct Rect {
    uint16_t x;
    int16_t y;
    uint16_t w;
    uint8_t h;
    uint8_t color;

    Rect(uint16_t x, int16_t y, uint16_t w, uint8_t h, uint8_t color) : x(x), y(y), w(w), h(h), color(color) {}
    Rect() = default;
};

class RectList {
    std::unique_ptr<Rect[]> rects;
    int max_list_size;
    int filled = 0;

public:
    RectList(int max) : rects(new Rect[max]), max_list_size(max) {}

    const Rect& operator[] (int idx) const {return rects[idx];}
    size_t size() const {return filled;}
    Rect* begin() {return rects.get();}
    Rect* end() {return rects.get() + filled;}

    void put(const Rect& r)
    {
        assert(filled+1 <= max_list_size);
        rects[filled++] = r;
    }

    void sub(const Rect& clip);
};


class PicBox {
    int w;
    int h;
    std::unique_ptr<uint8_t[]> img;

    void put_line(int x, int y, int length, Color color, bool clip=false);
    void put_box(int x, int y, int w, int h, Color color, bool clip=false);
    void draw_corner_box(char type, int corner, int x, int y, int r, Color color, int bwidth=0);

public:
    PicBox(int w, int h) : w(w), h(h), img(new uint8_t[w*h])
    {
        memset(img.get(), 0, w*h);
    }

    int get_h() const {return h;}
    int get_w() const {return w;}
    uint8_t* detach_img() {return img.release();}

    void draw_arc(int corner, int x, int y, int r, Color color, int bwidth=0);
};
        
        

class BoxCreator {
    RectList result;
    const int r;
    const int s;
    const int b;
    const int total_height;
    std::unique_ptr<uint8_t[]> images[4];

    using P2 = std::pair<int, int>;

    void draw_pic_box(PicBox& pic, int corner);
    void add_rect_list(const Rect& rect)
    {
        result.sub(rect);
        result.put(rect);
    }

    static int expect_rects(int r, int border_width, int shadow_width)
    {
        return (r ? 7 : 1) + (border_width ? 4 : 0) + (shadow_width ? 2 : 0);
    }

public:
    BoxCreator(int width, int height, int r, int border_width, int shadow_width);

    void draw(class LCD& lcd, int dx, int dy, const uint16_t* pallete, bool with_clip = true);
};
