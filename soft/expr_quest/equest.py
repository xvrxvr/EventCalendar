from random import randint, choice

def rand_bit() -> bool:
    return randint(0, 1) == 1

class CantGenerate(BaseException):
    pass

class GenSetup:
    def __init__(self, v_range):
        self.v_max = len(v_range)
        self.v_range = v_range
        self.v_abs_max = 10 ** self.v_max
        self.final_check = False

    def filter(self, selected: int) -> int:
        result = max(1, min(self.v_max, selected))
        return result

    def choose_digits(self) -> int:
        seq = []
        for idx, val in enumerate(self.v_range):
            seq += val * [idx]
        return choice(seq)+1

    def assert_max(self, val: int) -> bool:
        if val <=0 or val >= self.v_abs_max:
            return True
        return False

class Operation:
    def __init__(self, opcode: str, setup: GenSetup, left: 'Operation' = None):
        self.opcode = opcode
        self.setup = setup
        self.left = left

    @classmethod
    def build(cls, opcode: str, setup: GenSetup):
        F = {
            '+': OperationPlus,
            '-': OperationMinus,
            '*': OperationMul,
            '/': OperationDiv,
        }
        result = F[opcode](opcode, setup)
        return result

    ################## Virtual interface #########################
    def digits(self, selected: int) -> (int, int):
        assert False

    @property
    def prio(self):
        assert False

    def check(self):
        self.left.check()
        self.right.check()

    def fix(self, new_val: int) -> bool:
        assert False

    def get_val(self) -> int:
        assert False
    ##############################################################

    @property
    def values(self):
        return self.left.get_val(), self.right.get_val()

    def force(self, new_value: int) -> 'Operation':
        delta = new_value - self.get_val()
        if delta == 0:
            return self
        # print(f'*** Fix for {delta}')
        if delta > 0:
            new_op = OperationPlus('+', self.setup, self)
        else:
            new_op = OperationMinus('-', self.setup, self)
            delta = - delta
        new_op.right = LeafOp(self.setup, value=delta)
        return new_op

    # Also virtual
    def __str__(self):
        l = str(self.left)
        r = str(self.right)
        if self.left.prio < self.prio:
            l = f'({l})'
        if self.right.prio <= self.prio:
            r = f'({r})'
        return f'{l}{self.opcode}{r}'

class OperationPlus(Operation):
    def digits(self, selected: int) -> (int, int):
        return selected, selected

    @property
    def prio(self):
        return 1

    def get_val(self):
        return self.left.get_val() + self.right.get_val()

    def fix(self, new_val: int) -> bool:
        if self.setup.assert_max(new_val):
            return False
        l, r = self.values
        return self.left.fix(new_val - r) or self.right.fix(new_val - l)

class OperationMinus(Operation):
    def digits(self, selected: int) -> (int, int):
        return selected, selected

    @property
    def prio(self):
        return 1

    def get_val(self):
        l, r  = self.values
        diff = l - r
        if self.setup.final_check and diff <= 0:
            raise CantGenerate
        return diff

    def check(self):
        super().check()
        l, r = self.values
        dif = l - r
        if dif <= 0:
            if l < 2 and r < 2:
                raise CantGenerate
            ll = l // 4 + randint(0, l//2)
            rr = r + r // 2 + randint(0, r // 2)
            if rand_bit():
               if self.left.fix(rr) or self.right.fix(ll):
                   return
            else:
               if self.right.fix(ll) or self.left.fix(rr):
                   return
            # print('Minus')
            if rand_bit():
                self.left = self.left.force(rr)
            else:
                self.right = self.right.force(ll)

    def fix(self, new_val: int) -> bool:
        if self.setup.assert_max(new_val):
            return False
        l, r = self.values
        return self.left.fix(new_val + r) or self.right.fix(l - new_val)

class OperationMul(Operation):
    def digits(self, selected: int) -> (int, int):
        l_digit = self.setup.filter(selected//2)
        r_digit = self.setup.filter(selected - l_digit)
        return l_digit, r_digit

    @property
    def prio(self):
        return 2

    def get_val(self):
        return self.left.get_val() * self.right.get_val()

    def fix(self, new_val: int) -> bool:
        if self.setup.assert_max(new_val):
            return False
        l, r = self.values
        if (new_val % r) == 0 and self.left.fix(new_val // r):
            return True
        if (new_val % l) == 0 and self.right.fix(new_val // l):
            return True
        return False

class OperationDiv(Operation):
    def digits(self, selected: int) -> (int, int):
        l_digit = self.setup.filter(selected*2)
        r_digit = self.setup.filter(l_digit - selected)
        return l_digit, r_digit

    @property
    def prio(self):
        return 2

    def get_val(self):
        return self.left.get_val() // self.right.get_val()

    def check(self):
        super().check()
        l, r = self.values
        if (l % r) != 0:
            div = l // r
            if div < 2:
                raise CantGenerate
            if self.left.fix(div * r):
                return
            # print('Div')
            self.left = self.left.force(div * r)

    def fix(self, new_val: int) -> bool:
        if self.setup.assert_max(new_val):
            return False
        l, r = self.values
        if self.left.fix(new_val * r):
            return True
        if (l % new_val) == 0 and self.right.fix(l // new_val):
            return True
        return False

class LeafOp(Operation):
    def __init__(self, setup: GenSetup, selected: int = None, value: int = None):
        super().__init__('I', setup)
        if selected is not None:
            self.value = randint(10 ** (selected-1) + 1, 10 ** selected)
        else:
            if setup.assert_max(value):
                raise CantGenerate()
            self.value = value

    def get_val(self):
        return self.value

    @property
    def prio(self):
        return 3

    def check(self):
        pass

    def fix(self, new_val: int) -> bool:
        # print(f'Int fix {self.value} -> {new_val}')
        if self.setup.assert_max(new_val):
            return False
        self.value = new_val
        return True

    def __str__(self):
        return str(self.value)

class Generator(GenSetup):
    def _create_tree_shape(self, rest_opc: int, selected: int) -> (int, Operation):
        if rest_opc <= 0:
            return 0, LeafOp(self, selected)
        opc = choice('+-*/')
        result = Operation.build(opc, self)
        rest_opc -= 1
        l_digits, r_digits = result.digits(selected)
        if rand_bit():
            rest_opc, result.left = self._create_tree_shape(rest_opc, l_digits)
            rest_opc, result.right = self._create_tree_shape(rest_opc, r_digits)
        else:
            rest_opc, result.right = self._create_tree_shape(rest_opc, r_digits)
            rest_opc, result.left = self._create_tree_shape(rest_opc, l_digits)
        return rest_opc, result    

    def create_tree(self, total_opc: int):
        for _ in range(100):
            try:
                self.final_check = False
                tree = self._create_tree_shape(total_opc, self.choose_digits())[1]
                tree.check()
                self.final_check = True
                if not self.assert_max(tree.get_val()):
                    return tree
            except CantGenerate:
                # print('Regen')
                pass
        raise CantGenerate

"""
for _ in range(40):
#while True:
    tr = Generator((0,1,2)).create_tree(randint(1, 5))
    print(f'{tr} = {tr.get_val()}')
"""

LEVELS = [
    [(1,), 1, 1],   # 1
    [(2,1), 1, 1],  # 2
    [(1,2), 1, 2],  # 3
    [(1,2), 1, 3],  # 4
    [(0,2,1), 1, 3],  #5
    [(0,2,1), 2, 5],  #6
    [(0,1,2), 3, 5]   #7
]

while True:
    level = input("Select Level (1-7): ")
    level = int(level)
    if level < 1 or level > 7:
        print('Invalid level')
        continue
    level = LEVELS[level-1]
    tree = Generator(level[0]).create_tree(randint(level[1], level[2]))
    prompt = f'{tree} = '
    result = tree.get_val()
    while True:
        ans = input(prompt)
        if ans == '?':
            print(result)
            break
        if int(ans) == result:
            print('Yes!')
            break
        print('No...')
