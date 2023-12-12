import random
import heapq

class TileGameBoard:
    def __init__(self, v: int = None):
        if v is not None:
            self.value = v
        else:
            self.value = 0xFFFF
            for _ in range(100):
                self.invert(random.randint(0, 15))

    def __str__(self):
        acc = []
        val = self.value
        for c in range(4):
            s = ''
            for r in range(4):
                s += '* ' if (val & 1) != 0 else '. '
                val >>= 1
            acc.append('  |' + s.strip() + '|')
        return '\n'.join(acc)

    @property
    def solved(self):
        return self.value == 0xFFFF

    def invert(self, idx):
        mask = 0x1111 << (idx & 3)
        mask |= 0xF << (idx & 12)
        self.value ^= mask


def eval_weight(value: int):
    value ^= 0xFFFF
    acc = 0
    for _ in range(16):
        if value & 1:
            acc += 1
        value >>= 1
    return acc


class State:
    def __init__(self, value: TileGameBoard, parent: 'State', idx: int):
        self.value = value
        self.parent = parent
        self.plen = parent.plen + 1 if parent else 0
        self.idx = idx

    def update(self, new_parent: 'State'):
        if new_parent.plen < self.plen-1:
            self.parent = new_parent
            self.plen = new_parent.plen + 1

    def __lt__(self, other):
        return self.value.value < other.value.value

    def print_me(self):
        if not self.parent:
            return
        self.parent.print_me()
        print(f"Apply {self.idx&3}/{self.idx>>2}")
        print(self.value)
        print()

    def st_len(self):
        cnt = 0
        p = self
        while p:
            cnt += 1
            p = p.parent
        return cnt

class Solver:
    def __init__(self, root: TileGameBoard):
        self.tested = {}
        self.queue = []
        self.max_queue_len = 0
        self.add_item(State(root, None, -1))

    def add_item(self, state: State):
        if state.value.value in self.tested:
            self.tested[state.value.value].update(state)
        else:
            self.tested[state.value.value] = state
            weight = eval_weight(state.value.value) + state.plen
            #weight = state.plen
            heapq.heappush(self.queue, (weight, state))
            if len(self.queue) > self.max_queue_len:
                self.max_queue_len = len(self.queue)

    def process(self):
        while len(self.queue):
            weight, state = heapq.heappop(self.queue)
            for i in range(16):
                new_state = State(TileGameBoard(state.value.value), state, i)
                new_state.value.invert(i)
                if new_state.value.solved:
                    return  new_state
                self.add_item(new_state)


for _ in range(100):
    tg = TileGameBoard()
    #print(tg)

    sv = Solver(tg)

    res = sv.process()
    #res = print_me()
    print(f"Max Queue length: {sv.max_queue_len}, Items in state: {len(sv.tested)}, Solv Len: {res.st_len()}")


