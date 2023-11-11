from dataclasses import dataclass, asdict
import png

class Bres:
    def __init__(self, r):
        self.r = r

    def bres(self):
        x = 0
        y = self.r
        self.put_pnt(x, y)
        p = 3-2*self.r
        while y>x:
            if p < 0:
                p += 4*x+6
                x += 1
            else:
                p += 4*(x-y)+10
                x += 1
                y -= 1
            self.put_pnt(x, y)
        return self

class BresLine(Bres):
    """
        Generates upper right sector of circle

        r .
        ..
        ...
    .........
        0 .....

    """
    def __init__(self, r):
        super().__init__(r)
        self.result = (r+1) * [-1]   # y - index, x - value

    def put_pnt(self, x, y):
        self.result[y] = x
        if self.result[x] == -1:
            self.result[x] = y

class BresPoints(Bres):
    """
        Generates series of points (x, y)
    """
    def __init__(self, r):
        super().__init__(r)
        self.result = []

    def put_pnt(self, x, y):
        self.result.append((x, y))
        self.result.append((y, x))

def bres(r):
    return BresLine(r).bres().result

@dataclass
class BoxDef:
    sel: str
    x: int
    y: int
    w: int
    h: int
    data: any

@dataclass
class Rect:
    x: int
    y: int
    w: int
    h: int

    def __str__(self):
        return f'Rect(x={self.x},y={self.y},w={self.w},h={self.h}|x2={self.x+self.w-1},y2={self.y+self.h-1})'


class RectList:
    def __init__(self, rect, color):
        self.rects = [Rect(**asdict(rect))]
        self.color = color

    def __str__(self):
        acc = []
        for r in self.rects:
            acc.append(f"Rect {r}, Color={self.color}")
        return "\n".join(acc)

    def sub(self, clip):
        to_add = []
        print("Clip by rect: ", clip)
        for r in self.rects:
            x1, y1, x2, y2 = r.x, r.y, r.x+r.w, r.y+r.h
            xx1, yy1, xx2, yy2 = clip.x, clip.y, clip.x+clip.w, clip.y+clip.h
            if x1 >= xx2 or y1 >= yy2 or x2 <= xx1 or y2 <= yy1:
                continue
            xx1 = min(x2, max(x1, xx1))
            xx2 = min(x2, max(x1, xx2))
            yy1 = min(y2, max(y1, yy1))
            yy2 = min(y2, max(y1, yy2))
            """
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

            """
            fit = (1 if x1 == xx1 else 0) | (2 if x2 == xx2 else 0) | (4 if y1 == yy1 else 0) | (8 if y2 == yy2 else 0)
            print("Fit=", fit)
            assert fit !=0
            if fit == 15:
                continue
            if fit == 15-1:   # cut right part
                x2 = xx1
            elif fit == 15-2:   # cur left part
                x1 = xx2
            elif fit == 15 - 8: # cut bottom part
                y1 = yy2
            elif fit == 15 - 4: # cur top part
                y2 = yy1
            elif fit == 5:  # cut left bottom - split
                to_add.append(Rect(xx2, y1, x2 - xx2, yy2 - y1))
                y1 = yy2
            elif fit == 6:  # cut right bottom - split
                to_add.append(Rect(x1, y1, x1 - xx1, yy2 - y1))
                y1 = yy2
            elif fit == 9:  # cur top left - split
                to_add.append(Rect(xx2, yy1, x2 - xx2, y2 - yy1))
                y2 = yy1
            elif fit == 10:  # cur top right - split
                to_add.append(Rect(x1, yy1, x1 - xx1, y2 - yy1))
                y2 = yy1
            else:
                assert False, f"Unsupported fit value {fit}"
            r.x, r.y = x1, y1
            r.w, r.h = x2-x1, y2-y1
            print("Clip result: ", r)
        self.rects += to_add
        for ta in to_add:
            print("Added ", ta)


class PicBox:
    def __init__(self, w, h):
        self.w = w
        self.h = h
        self.img = [ w * [0] for _ in range(h) ]

    def put_line(self, x, y, lenght, color, clip=False):
        if lenght < 0:
            x += lenght + 1
            lenght = -lenght
        if clip and (y< 0 or y>= self.h):
            return
        assert y>= 0        
        img = self.img[self.h - y - 1]
        for _ in range(lenght):
            if not clip or not (x<0 or x >= self.w):
                assert x >= 0
                img[x] = color
            x += 1

    def put_box(self, x, y, w, h, color, clip=False):
        if h < 0:
            y += h + 1
            h = -h
        for _ in range(h):
            if not clip or not (y < 0 or y >= self.h):
                self.put_line(x, y, w, color, clip)
            y += 1

    def draw_corner_box(self, type, corner, x, y, r, color, bwidth=0):
        if type == '-':
            return
        w = x if (corner & 1) == 0 else self.w - x
        h = y if (corner & 2) else self.h - y
        bt = ''
        if type == 'H':
            h = r
            if (corner & 2) == 0:
                h += 1
                bt = 'T'
            else:
                bt = 'B'
        elif type == 'V':
            w = r
            if (corner & 1):
                w += 1
                bt = 'R'
            else:
                bt = 'L'
        if (corner & 2):
            h = -h
            y -= 1
        if (corner & 1) == 0:
            w = -w
            x -= 1
        if w and h:
            if h < 0:
                y += h + 1
                h = -h
            if w < 0:
                x += w + 1
                w = -w
            self.put_box(x, y, w, h, color)
            if bwidth:
                if bt == 'R':
                    self.put_box(x + w - bwidth, y, bwidth, h, 2)
                elif bt == 'L':
                    self.put_box(x, y, bwidth, h, 2)
                elif bt == 'T':
                    self.put_box(x, y+ h - bwidth, w, bwidth, 2)
                elif bt == 'B':
                    self.put_box(x, y, w, bwidth, 2)

    def draw_arc(self, corner, x, y, r, color, bwidth=0):
        arc = bres(r)
        delta_y = -1 if corner & 2 else 1
        delta_x = 1 if corner & 1 else -1
        yy = y
        for bx in arc:
            self.put_line(x, yy, bx*delta_x, color)
            yy += delta_y
        if bwidth:
            brd = BresPoints(r).bres()
            # Generate series of boxes from points of Arc and edge length <border-width*sqrt(2)>
            # bw = (bwidth*361) >> 8  # (1.41)
            bw = bwidth
            dx = -bw*delta_x
            dy = -bw*delta_y
            for arc_x, arc_y in brd.result:
                xx = x + arc_x*delta_x
                yy = y + arc_y*delta_y
                self.put_box(xx, yy, dx, dy, 2, clip=True)
        self.draw_corner_box("-HVF"[corner], 0, x, y, r, color, bwidth)
        self.draw_corner_box("H-FV"[corner], 1, x, y, r, color, bwidth)
        self.draw_corner_box("VF-H"[corner], 2, x, y, r, color, bwidth)
        self.draw_corner_box("FVH-"[corner], 3, x, y, r, color, bwidth)

    def draw_circle(self, x, y, r, color):
        arc = bres(r)
        for by, bx in enumerate(arc):
            self.put_line(x+bx, y + by, 1, color)
        
        

"""
    Returns rounded box definition:
    array of pic/blocks:
    P: X + Y, W x H x Bitmap
    F: X + Y, W x H, Color
    Colors:
        0 - Not fill
        1 - Body
        2 - Border
        3 - Shadow
"""

class BoxCreator:
    def __init__(self, width, height, r, border_width, shadow_width):
        """
        Creates Box with border, rounded corners and shadow
        Internally all coordinates are relatives to box bottom/left corner. If shadow requested its Y coordinate is negative.
        On BoxDef emit they recalculated

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

        """
        self.result = []
        self.r = r
        self.s = shadow_width
        self.b = border_width

        self.rect_list = [RectList(Rect(0, 0, width, height), 1)]  # List of all Rect to emit. Initialized it with Whole Window
        print(f"Root box: {self.rect_list[0]}")

        if border_width:
            self.add_rect_list(Rect(0, 0, width, border_width), 2)
            self.add_rect_list(Rect(0, height-border_width, width, border_width), 2)
            self.add_rect_list(Rect(0, 0, border_width, height), 2)
            self.add_rect_list(Rect(width-border_width, 0, border_width, height), 2)

        if shadow_width:
            self.add_rect_list(Rect(shadow_width, -shadow_width, width, shadow_width), 3)
            self.add_rect_list(Rect(width, -shadow_width, shadow_width, height), 3)

        if r:
            box_size = r + 1 + shadow_width
            corn_encode = (
                (0, height - box_size),
                (width + shadow_width - box_size , height - box_size),
                (0, -shadow_width),
                (width + shadow_width - box_size , -shadow_width)
            )
            for bidx in range(4):
                pic = PicBox(box_size, box_size)
                self.draw_pic_box(pic, bidx)
                x, y = corn_encode[bidx]
                self.add_rect_list(Rect(x, y, box_size, box_size), pic.img)

    def get_result(self):
        result = []
        for rl in self.rect_list:
            for rect in rl.rects:
                if isinstance(rl.color, list):
                    result.append(BoxDef('S', rect.x, rect.y, rect.w, rect.h, rl.color))
                else:
                    result.append(BoxDef('F', rect.x, rect.y, rect.w, rect.h, rl.color))
        return result

    def draw_pic_box(self, pic, corner):
        w, h = pic.w-1, pic.h-1
        r = self.r
        x, y = (
            (r, h-r),
            (0,   h-r),
            (r,   h),
            (0,   h)
        )[corner]
        if self.s and corner != 0:
            pic.draw_arc(corner, x+self.s, y-self.s, r, 3)
        pic.draw_arc(corner, x, y, r, 1, self.b)


    def add_rect_list(self, rect, color):
        for l in self.rect_list:
            l.sub(rect)
        self.rect_list.append(RectList(rect, color))
        print("Add rect", rect, "with Color=", color)


class Picture:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.img = [ (width * 3) * [255] for _ in range(height) ]

    def put_px(self, x, y, color):
        if not color:
            return
        #assert self.height-y-1 >= 0
        assert y >= 0
        assert x*3+color-1 >= 0
        self.img[y][x*3+color-1] = 0


    def draw_boxes(self, boxes, height, dy=0):
        for box in boxes:
            box_y = height - dy - box.y - box.h
            if box.sel == 'F':
                for x in range(box.x, box.x+box.w):
                    for y in range(box_y, box_y+box.h):
                        self.put_px(x, y, box.data)
            else:
                for x in range(box.w):
                    for y in range(box.h):
                        self.put_px(box.x+x, box_y+y, box.data[y][x])

    def gen_png(self, fname):
        with open(fname, 'wb') as f:
            w = png.Writer(self.width, self.height, greyscale=False)
            w.write(f, self.img)


def prn(res):
    for v in reversed(res):
        print(v * '*')

pic = Picture(400, 240)

shadow = 5
height = 100

pic.draw_boxes(BoxCreator(width=200, height=height, r=15, border_width=2, shadow_width=shadow).get_result(), height+shadow, shadow)

pic.gen_png('test.png')
