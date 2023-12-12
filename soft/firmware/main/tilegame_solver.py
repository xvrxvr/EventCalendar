import random

def invert(value, idx):
    mask = 0x1111 << (idx & 3)
    mask |= 0xF << (idx & 12)
    return value ^ mask


class GenSolver:
    def __init__(self):
        self.states = 65536 * [None]
        self.queue = []
        self.st_max_len = 0
        self.st_total = 0
        self.st_total_length = 0
        self.push(0xFFFF, 0, 0)
        while self.queue:
            val, idx, length = self.queue.pop(0)
            self.push(val, idx, length)


    def push(self, value:int, index:int, length:int):
        if self.states[value] is not None:
            return
        self.states[value] = index
        self.add_stat(length)
        for i in range(16):
            new_val = invert(value, i)
            if self.states[new_val] is None:
                self.queue.append((new_val, i, length+1))

    def add_stat(self, length):
        self.st_max_len = max(self.st_max_len, length)
        self.st_total += 1
        self.st_total_length += length

    @property
    def avg_length(self):
        return self.st_total_length / self.st_total

    def write_src(self):
        print(f'// Total values: {self.st_total}, Max length: {self.st_max_len}, Avg length: {self.avg_length}')
        print(f"""#include "common.h"
        namespace TileGame {{
        extern const int max_slv_length = {self.st_max_len};
        extern const uint8_t solver_table[32768] = {{""")

        it = iter(self.states)
        for idx, (low, hi) in enumerate(zip(it, it)):
            if (idx & 15) == 0:
                print('    ', end='')
            print(f"0x{low | (hi<<4):02X}{',' if idx != 32767 else ''} ", end='')
            if (idx & 15) == 15:
                print()

        print("};\n} // namespace TileGame")

    def test_value(self, val):
        print(f'{val:04X} ', end='')
        for _ in range(self.st_max_len):
            idx = self.states[val]
            val = invert(val, idx)
            print(f'[x={idx&3}, y={idx>>2}] {val:04X} ', end='')
            if val == 0xFFFF:
                print()
                return
        assert False
        

sv = GenSolver()

#for v in range(100):
#    sv.test_value(random.randint(0, 0xFFFF))

sv.write_src()

