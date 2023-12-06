with open('hlp.txt', 'r') as f:
    for l in f:
        acc = 0
        for s in l.strip():
            acc <<= 1
            if s == '*':
                acc |= 1
        print(f'0x{acc:08X}, ', end='')
