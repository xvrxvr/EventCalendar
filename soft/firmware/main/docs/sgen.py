import re

Lines = []

def dump_lines():
    assert len(Lines) == 16, Lines
    chars = []
    for l in Lines:
        assert len(l) % 8 == 0
        sym = 0
        chars2 = []
        for idx, s in enumerate(l):
            sym <<= 1
            if s == '*':
                sym |= 1
            if idx % 8 == 7:
                chars2.append(sym)
                sym = 0
        chars.append(chars2)
    total = len(chars[0])
    for col in range(total):
        for ch in chars:
            print(f"0x{ch[col]:02X}, ", end="")
        print()

def new_sym(name):
    global Lines
    if Lines:
        dump_lines()
        Lines = []
    print("Symbol", name)


with open('syms.txt', 'r') as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        line = line.replace(' ', '')
        if re.match('[^.*]', line):
            new_sym(line)
        else:
            Lines.append(line)

dump_lines()
