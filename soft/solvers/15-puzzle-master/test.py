import intelligence
import board


for _ in range(100):
    b = board.Board()
    b.scramble(10)

    res = intelligence.a_star(b, intelligence.manhattan_heuristic)
    print (res[1])
