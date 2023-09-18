# X: 816 688 601 568 580 559 544 528 531 528
# ~X: 2609 2595 2608 2608 2588 2608 2588 2615 2603 2612
# Y: 2615 2606 2607 2614 2599 2605 2582 2586 2587 2606
# ~Y: 256 264 255 254 259 240 247 255 243 254

lines = []    

xpairs = []
ypairs = []

# print("  X  |  ~X |X+ ~X|  Y  |  ~Y |Y+ ~Y")

with open("log.log", "r") as t:
    for l in t:
        toks = l.split()
        if not toks:
            for x, nx, y, ny in zip(*lines):
                # print(f"{x:5}|{nx:5}|{x+nx:5}|{y:5}|{ny:5}|{y+ny:5}")
                xpairs.append((x, nx))
                ypairs.append((y, ny))
            lines = []
        else:
            lines.append([int(x) for x in toks[1:]])

def prn(title, array):
    array.sort()
    print(title)
    for x, nx in array:
        t = x + nx
        s = ((t - 2700) * 120 // 200) * '*'
        print(f'{x:5}|{nx:5}|{t:5}| {s}')

prn("X:", xpairs)
prn("Y:", ypairs)
    