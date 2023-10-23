import sys

import reversi_misc
import reversi_solver_misc

results = dict()
memo = dict()
memolist = []


def dfs(player: int, opponent: int) -> tuple[int, int]:

    assert (player & opponent) == 0
    assert reversi_misc.board_unique(player, opponent) == (player, opponent)

    if (player, opponent) in memo:
        return memo[(player, opponent)]

    n_empties = 64 - player.bit_count() - opponent.bit_count()
    bb_moves = reversi_misc.get_moves(player, opponent)

    if bb_moves > 0 and n_empties == 50:
        if (player, opponent) in results:
            return results[(player, opponent)]
        return (-64, 64)

    if bb_moves == 0:
        if reversi_misc.get_moves(opponent, player) == 0:
            diff = player.bit_count() - opponent.bit_count()
            if diff > 0:
                score = diff + n_empties
            elif diff < 0:
                score = diff - n_empties
            else:
                score = 0
            return (score, score)
        else:
            (np, no) = reversi_misc.board_unique(opponent, player)
            return dfs(np, no)

    score_min, score_max = -64, -64
    for square in [x for x in range(64) if ((1 << x) & bb_moves) > 0]:
        flipped = reversi_misc.flip(square, player, opponent)
        next_player = opponent ^ flipped
        next_opponent = player ^ flipped ^ (1 << square)
        (np, no) = reversi_misc.board_unique(next_player, next_opponent)
        (a, b) = dfs(np, no)
        score_min = max(score_min, -b)
        score_max = max(score_max, -a)

    assert reversi_misc.board_unique(player, opponent) == (player, opponent)
    memo[(player, opponent)] = (score_min, score_max)
    memolist.append(
        (reversi_misc.bitboards_to_obf(player, opponent), score_min, score_max)
    )
    return (score_min, score_max)


def fill_results():
    tasklist = reversi_solver_misc.read_empty50_tasklist_edax_knowledge()
    for obf in tasklist:
        with open(f"result_e50{obf[:64]}.csv", "r") as f:
            lines = [s.strip() for s in f.readlines()]
            assert lines[0] == "obf,is_exact,score_lowerbound,score_upperbound"
            xs = lines[1].split(",")
            assert len(xs) == 4
            if xs[1] != "1":
                #unsolved. error. exit.
                print(f"error: there is an unsolved e50 problem: {obf}. exit.")
                sys.exit(0)
            results[reversi_misc.obf_to_bitboards(reversi_misc.obf_unique(xs[0]))] = (
                int(xs[2]),
                int(xs[3]),
            )
            memolist.append((reversi_misc.obf_unique(xs[0]), int(xs[2]), int(xs[3])))


if __name__ == "__main__":
    fill_results()
    player, opponenet = reversi_misc.initial_position("cross")
    dfs(player, opponenet)
    memolist.reverse()
    with open("result_e50_opening_book.csv", "w") as f:
        f.write("obf,score_lowerbound,score_upperbound\n")
        for x in memolist:
            f.write(f"{x[0]},{x[1]},{x[2]}\n")
    print("make_50_book.py: successfully finished.")
