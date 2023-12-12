import random

class Point:
    def __init__(self, x=0, y=0):
        self.x = x
        self.y = y

    @property
    def idx(self):
        return self.x + self.y*4

    def __add__(self, p: 'Point'):
        return Point(self.x + p.x, self.y + p.y)

    def __eq__(self, p: 'Point'):
        return self.x == p.x and self.y == p.y

class Rect:
    def __init__(self, p1, p2):
        self.p1 = p1
        self.p2 = p2

    def distance(self, p: Point, direction: str):
        """
            Return distance form point 'p' to our rect in direction 'direction' (up/dn/rt/lt)
        """
        if direction in ('up', 'dn'):
            if p.y < self.p1.y:
                return self.p1.y - p.y
            elif p.y > self.p2.y:
                return p.y - self.p2.y
            else:
                return 0
        else:
            if p.x < self.p1.x:
                return self.p1.x - p.x
            elif p.x > self.p2.x:
                return p.x - self.p2.x
            else:
                return 0

    @property
    def perimeter(self):
        return 2*(self.p2.x-self.p1.x+1 + self.p2.y-self.p1.y+1)

    def distance_on_path(self, p: Point):
        """
            Distance from top/left corner of rect to point (clockwise)
        """
        result = 0
        if p.y == self.p1.y:
            result += p.x-self.p1.x
            return result
        result += self.p2.x-self.p1.x1
        if p.x == self.p2.x:
            result += p.y-self.p1.y
            return result
        result += self.p2.y-self.p1.y+1
        if p.y == self.p2.y:
            result += self.p2.x - p.x
            return result
        result += self.p2.x - self.p1.x1
        assert p.x == self.p1.x
        result += self.p2.y - p.y
        return result


    def where(self, p: Point):
        """
            Test where our p on path
        """
        selector = 0
        if p.x == self.p1.x:
            selector |= 1
        if p.x == self.p2.x:
            selector |= 2
        if p.y == self.p1.y:
            selector |= 4
        if p.y == self.p2.y:
            selector |= 8
        return [
            '',   # 0
            'l',  # 1
            'r',  # 2
            '?',  # 3
            't',  # 4
            'tl', # 5
            'tr', # 6
            '?',  # 7
            'd',  # 8
            'dl', # 9
            'dr', # A
            '?',  # B
            '?',  # C
            '?',  # D
            '?',  # E
            '?',  # F
        ] [selector]


    def distance_on_edge(self, direction: str, p: Point):
        """
            Return distance from Box boundary to p in 'direction' along the edge (from corner to point)
        """
        if direction == 'rt': # from left X to point
            return p.x - self.p1.x
        if direction == 'lt': # from right X to point
            return self.p2.x - p.x
        if direction == 'dn': # from top Y to point
            return p.y - self.p1.y
        if direction == 'up': # from bottom Y to point
            return self.p2.y - p.y


    def test_on(self, p: Point):
        """
            Test if point 'p' layed on our Rect
        """
        return self.where(p) != ''
                

    @classmethod
    def wrap(cls, *args):
        """
            Takes series of Point and wrap all of them in one Rect
        """
        min_x = min((p.x for p in args))
        max_x = max((p.x for p in args))
        min_y = min((p.y for p in args))
        max_y = max((p.y for p in args))
        return Rect(Point(min_x, min_y), Point(max_x, max_y))


class Game15:
    def __init__(self):
        self.board = [1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,0]
        self.verbose = False
        self.shuffle()
        self.filled_lines = 0
        self.filled_in_line = 0

    def __str__(self):
        result = []
        for y in range(4):
            acc = ' |'
            for x in range(4):
                val = self.board[x+y*4]
                acc += f'{val:2d} ' if val else '   '
            result.append(acc + '|')
        return " +------------+\n" + "\n".join(result) + "\n +------------+\n"

    def get(self, p: Point):
        return self.board[p.idx]

    def find(self, num):
        idx = self.board.index(num)
        return Point(idx & 3, idx >> 2)

    def move(self, direction: str, how = 1):
        if self.verbose:
            print(f'Move: {direction} by {how}')
        p = self.find(0)
        dd = {'up': Point(0, 1), 'dn': Point(0, -1), 'lt': Point(1, 0), 'rt': Point(-1, 0)} [direction]
        for _ in range(how):
            nn = p + dd
            if not (0 <= nn.x <= 3 and 0 <= nn.y <= 3):
                if self.verbose:
                    print("! Move terminated!")
                break
            self.board[p.idx] = self.board[nn.idx]
            self.board[nn.idx] = 0
            p = nn
        if self.verbose:
            print(self)


    def shuffle(self):
        for _ in range(100):
            self.move(random.choice(('up', 'dn', 'lt', 'rt')), random.randint(1,4))

    # Solver
    def solver_move_cycle(self, dst_position: Point, src_value: int, arena: 'Arena'):
        """
            Move value 'src_value' to position dst_position
            All moves performed in 'arena'
        """
        if self.get(dst_position) == src_value:
            return
        hole = self.find(0)
        val = self.find(src_value)
        if not arena.belong_to(hole):
            # Move hole to our arena
            direction = arena.move_dir(hole)
            if arena.is_leave_on_move(direction, hole, val):
                # We can't just move Hole inside - our source point will be shifed out of Arena
                premove = arena.premove(direction, hole)
                print("Premove Hole")
                self.move(premove)
                hole = self.find(0)
                val = self.find(src_value)
            our_rect = arena.rect(dst_position, val)
            print("Move Hole into Arena")
            self.move(direction, our_rect.distance(hole, direction))
            hole = self.find(0)
            val = self.find(src_value)
        # Our path to move Value (as Rect)
        path = arena.rect(dst_position, val, hole)
        if not path.test_on(val):
            # We lost Value to shift - it resides somewhere inside our Rect. Diascard Hole
            path = arena.rect(dst_position, val)
            assert path.test_on(val)
        path.test_on(dst_position)
        if not path.test_on(hole):
            # Our hole somewhere inside or outside, but not intersect with Dst or value - move it to arbitrary position on Path
            print("Move Hole on Path")
            if hole.x < path.p1.x:
                self.move('lt', path.p1.x - hole.x)
            else:
                self.move('rt', hole.x - path.p1.x)
            hole = self.find(0)
            val = self.find(src_value)
            path = arena.rect(dst_position, val, hole)
        assert path.test_on(dst_position) and path.test_on(val) and path.test_on(hole)

        # Now move our Path in cycles
        if path.dist_on_path(val) <= path.perimeter//2:
            # Move val Unticlockwise
            # Path rigth&right/top: up, Path down&down/right: rt, Path left&left/down: dn, Path top&top/left: lt
            print("CCW run")
            dirs = {
                'r': 'up', 'tr': 'up',
                'd': 'rt', 'dr': 'rt',
                'l': 'dn', 'dl': 'dn',
                't': 'lt', 'tl': 'lt'
            }
        else:
            # Move colckwise
            # Part right&right/down: dn, Path top&top/right: rt, Path left&left/top: up, Path down&down/left: rt
            print("CW run")
            dirs = {
                'r': 'dn', 'dr': 'dn',
                'd': 'lt', 'dl': 'lt',
                'l': 'up', 'tl': 'up',
                't': 'rt', 'tr': 'rt'
            }
        # Let's run
        for _ in range(64):
            assert path.test_on(hole)
            cur_dir = dirs[path.where(hole)]
            self.move(cur_dir, path.distance_on_edge(cur_dir, hole))
            hole = self.find(0)
            if self.get(dst_position) == src_value:
                return
            path = arena.rect(dst_position, val, hole)  # Path could be shrinked - reevaluate it
        assert False

        

class Arena:
    """
        Space where we can move our tiles
    """
    def __init__(self, r: Rect):
        self.r = r

    def belong_to(self, p: Point):
        return self.r.p1.x <= p.x <= self.r.p1.x and self.r.p1.y <= p.y <= self.r.p1.y

    def move_dir(self, p: Point):
        """
            How we need to move Hole (x,y) to move it into our arena
        """
        if self.r.p1.x <= p.x <= self.r.p1.x:
            if p.y < self.r.p1.y:
                return 'up'
            else:
                return 'dn'
        if self.r.p1.y <= y <= self.r.p1.y:
            if p.x < self.r.p1.x:
                return 'lt'
            else:
                return 'rt'
        assert False    

    def is_leave_on_move(self, direction: str, hole: Point, p: Point):
        """
            Test if our point 'p' will leave our Arena if we move Hole ('hole') in direction 'Direction'
        """
        if direction == 'up' and hole.x == p.x:
            return p.y-1 < self.r.p1.y
        if direction == 'dn' and hole.x == p.x:
            return p.y+1 > self.r.p2.y
        if direction == 'lt' and hole.y == p.y:
            return p.x-1 < self.r.p1.x
        if direction == 'rt' and hole.y == p.y:
            return p.x+1 > self.r.p2.x
        return False

    def premove(self, direction: str, hole: Point):
        """
            How we should move Hole to enable move them into Arena and not shifted out our Value
        """
        if direction in ('up', 'dn'):
            # Our final move will be vertical - so move now horizontaly
            if hole.x-1 >= self.r.p1.x:
                return 'rt'
            else:
                return 'lt'
        else:
            if hole.y-1 >= self.r.p1.y:
                return 'dn'
            else:
                return 'up'

    def rect(self, *args):
        result = Rect.wrap(*args)
        if result.p1.x == result.p2.x:
            if self.p2.x >= result.p2.x+1:
                result.p2.x += 1
            elif self.p1.x <= result.p1.x-1:
                result.p1.x-=1
            else:
                assert False
        elif result.p1.y == result.p2.y:
            if self.p2.y >= result.p2.y+1:
                result.p2.y += 1
            elif self.p1.y <= result.p1.y-1:
                result.p1.y-=1
            else:
                assert False
        return result

g = Game15()
print(g)
g.verbose=True
g.move('up')

