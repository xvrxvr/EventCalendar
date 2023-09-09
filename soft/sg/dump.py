with open("8X16.IMG", "rb") as f:
    all = f.read()

sg = []
for i in range(256):
    sg.append(all[i*16 : i*16+16])

for idx, s in enumerate(all):
    print(f'0x{s:02x},', end='')
    if (idx % 16) == 15:
        print()

#print(sg)

for sym in sg:
    # One symbol - 16 lines
    for ln_bin in sym:
        ln = ''
        for i in range(8):
            ln += '*' if (ln_bin << i) & 0x80 else ' '
        print(ln)
    print()

        


