import os
import random
import re
import subprocess
from typing import Optional, Tuple

import reversi_misc
import reversi_solver_misc


def print_obf(obf: str) -> None:
    player, opponent = reversi_misc.obf_to_bitboards(obf)
    bb_moves = reversi_misc.get_moves(player, opponent)
    if bb_moves == 0:
        assert reversi_misc.get_moves(opponent, player) == 0
    assert ((player | opponent) & bb_moves) == 0
    print("  A B C D E F G H")
    for i in range(8):
        line = f"{i+1} "
        for j in range(8):
            line += obf[i * 8 + j] if (bb_moves & (1 << (i * 8 + j))) == 0 else "."
            line += " "
        suffix = ""
        if i == 1:
            suffix = f" {obf[65]} to move" if bb_moves > 0 else " game is end."
        elif i == 3:
            suffix = f" O: discs = {str(obf[:64].count('O')).rjust(2)}"
        elif i == 4:
            suffix = f" X: discs = {str(obf[:64].count('X')).rjust(2)}"
        elif i == 5:
            suffix = f"  empties = {str(obf[:64].count('-')).rjust(2)}"
        print(line + str(i + 1) + suffix)
    print("  A B C D E F G H")


def simple_solver(player: int, opponent: int, depth: int) -> Tuple[int, str]:
    bb_moves = reversi_misc.get_moves(player, opponent)
    if bb_moves == 0:
        if reversi_misc.get_moves(opponent, player) == 0:
            return (reversi_misc.ComputeFinalScore(player, opponent), "end")
        if depth == 0:
            return (-100, "unknown")
        score = simple_solver(opponent, player, depth)[0]
        if score == -100:
            return (-100, "ps")
        return (-score, "ps")
    if depth == 0:
        return (-100, "unknown")
    best_score = -100
    best_move = ""
    for move in [i for i in range(64) if (bb_moves & (1 << i)) > 0]:
        flipped = reversi_misc.flip(move, player, opponent)
        assert flipped > 0
        score, _ = simple_solver(
            opponent ^ flipped, player ^ flipped ^ (1 << move), depth - 1
        )
        if score == -100:
            continue
        score = -score
        if score > best_score:
            best_score = score
            best_move = reversi_misc.index2str(move)
    return (best_score, best_move)


def simple_solver_root(
    player: int, opponent: int, depth: int = 3
) -> Optional[Tuple[int, str]]:
    score, move = simple_solver(player, opponent, depth)
    if score > 0:
        return (score, move)
    return None


def deploy_one_problem_to_edax(obf, n_cores):
    assert re.fullmatch(r"[-OX]{64}\s[OX];", obf) is not None
    obffilename = "obf.txt"
    with open(obffilename, "w") as f:
        f.write(obf + "\n")
    ab = 64 if obf.count("-") < 16 else 1
    cmd = f"stdbuf -oL ./Edax_mod2 -solve {obffilename} -n-tasks {n_cores} -level 60 -hash-table-size 23 -verbose 2 -alpha -{ab} -beta {ab} -width 200"
    proc = subprocess.Popen(
        cmd.strip().split(" "),
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        universal_newlines=True,
        bufsize=0,
    )
    result_stdout = []
    while True:
        line = proc.stdout.readline()
        if not line and proc.poll() is not None:
            break
        if line:
            result_stdout.append(line.rstrip())
    return reversi_solver_misc.compute_one_verbose2_problem(result_stdout)[
        "principal_variation"
    ][0]


def search_best_move_from_table(filename, player, opponent):
    bb_moves = reversi_misc.get_moves(player, opponent)
    nega_max_next_result = -100
    best_i = None
    for i in range(64):
        if (bb_moves & (1 << i)) > 0:
            flipped = reversi_misc.flip(i, player, opponent)
            assert flipped > 0
            next_player = opponent ^ flipped
            next_opponent = player ^ flipped ^ (1 << i)
            next_bb_moves = reversi_misc.get_moves(next_player, next_opponent)
            pass_flag = 1
            if next_bb_moves == 0:
                if reversi_misc.get_moves(next_opponent, next_player) == 0:
                    continue
                else:
                    next_player, next_opponent = next_opponent, next_player
                    pass_flag = -1
            next_obf = reversi_misc.bitboards_to_obf(next_player, next_opponent)
            next_obf_unique = reversi_misc.obf_unique(next_obf)
            next_query = reversi_misc.obf_to_base81encoding(next_obf_unique)
            next_result = reversi_misc.read_table(next_query, filename)
            if next_result is not None:
                if nega_max_next_result < -int(next_result) * pass_flag:
                    nega_max_next_result = -int(next_result) * pass_flag
                    best_i = i
    return best_i


def get_random_move(player, opponent, random_seed=None):
    bb_moves = reversi_misc.get_moves(player, opponent)
    assert bb_moves > 0
    movelist = [i for i in range(64) if (bb_moves & (1 << i)) > 0]
    if random_seed is not None:
        random.seed(random_seed)
    answer = reversi_misc.index2str(random.choice(movelist))
    print(f"info: random_choice = {answer}")
    return answer


def get_optimal_move(player, opponent):
    if bin(player | opponent).count("1") == 4:
        return "f5"
    dfs_answer = simple_solver_root(player, opponent)
    if dfs_answer is not None:
        print(f"info: dfs_answer = {dfs_answer[1]}")
        assert re.fullmatch(r"[a-h][1-8]", dfs_answer[1]) is not None
        return dfs_answer[1]
    filename = "all_result_abtree_encoded_sorted_unique.csv"
    table_answer = search_best_move_from_table(filename, player, opponent)
    if table_answer is not None:
        print(f"info: table_answer = {reversi_misc.index2str(table_answer)}")
        return reversi_misc.index2str(table_answer)
    if bin(player | opponent).count("1") < 64 - 36:
        print(
            "warning: There are 37 or more empty squares, but the table could not provide the answer.\n"
            '         We assume that you must have used "random" or manually option for both players.\n'
            '         It must have caused the position to fall outside the scope of what we must cover in order to satisfy the "weakly solved".\n'
            "         Otherwise, our table or code is wrong.\n"
            'warning: fall-back to the "random".'
        )
        return get_random_move(player, opponent)
    n_workers = min(max(1, os.cpu_count()), 8)
    edax_answer = deploy_one_problem_to_edax(
        reversi_misc.bitboards_to_obf(player, opponent), n_workers
    )
    assert re.fullmatch(r"[a-h][1-8]", edax_answer) is not None
    print(f"info: edax_answer = {edax_answer}")
    return edax_answer


def gamerecord_to_obf(gamerecord: str) -> str:
    assert re.fullmatch(r"([a-hA-H][1-8]){0,60}", gamerecord) is not None
    black, white = reversi_misc.initial_position("cross")
    player_is_black = True
    moves = [gamerecord[i : i + 2].lower() for i in range(0, len(gamerecord), 2)]
    for move in moves:
        player, opponent = (black, white) if player_is_black else (white, black)
        bb_moves = reversi_misc.get_moves(player, opponent)
        if bb_moves == 0:
            if reversi_misc.get_moves(opponent, player) == 0:
                assert False
            player_is_black = not player_is_black
            player, opponent = opponent, player
            bb_moves = reversi_misc.get_moves(player, opponent)
        square_index = reversi_misc.str2index(move)
        assert (bb_moves & (1 << square_index)) > 0
        flipped = reversi_misc.flip(square_index, player, opponent)
        assert flipped > 0
        next_player = opponent ^ flipped
        next_opponent = player ^ flipped ^ (1 << square_index)
        black, white = (
            (next_opponent, next_player)
            if player_is_black
            else (next_player, next_opponent)
        )
        player_is_black = not player_is_black
    player, opponent = (black, white) if player_is_black else (white, black)
    bb_moves = reversi_misc.get_moves(player, opponent)
    if bb_moves == 0 and reversi_misc.get_moves(opponent, player) != 0:
        player_is_black = not player_is_black
        player, opponent = opponent, player
    return reversi_misc.bitboards_to_obf(player, opponent, player_is_black)


def player_root(random_playout=None):
    if random_playout is not None:
        random_is_black = random.randint(0, 1) == 0
        branch_threhsold = random.randint(4, 16)
    black, white = reversi_misc.initial_position("cross")
    player_is_black = True
    gamerecord = ""
    while True:
        now_obf = gamerecord_to_obf(gamerecord)
        print(f"info: {now_obf=}")
        print_obf(now_obf)
        player, opponent = (black, white) if player_is_black else (white, black)
        bb_moves = reversi_misc.get_moves(player, opponent)
        if bb_moves == 0:
            if reversi_misc.get_moves(opponent, player) == 0:
                if bin(black).count("1") > bin(white).count("1"):
                    print("info: game is end. black wins.")
                elif bin(black).count("1") < bin(white).count("1"):
                    print("info: game is end. white wins.")
                else:
                    print("info: game is end. draw.")
                return
            player_is_black = not player_is_black
            player, opponent = (black, white) if player_is_black else (white, black)
        if gamerecord != "":
            print(f"info: {gamerecord=}")
        print(
            f'Input a square to make a move manually. (e.g. "f5") Input "solve" to do automatically. Input "random" to do randomly.'
        )
        while True:
            if random_playout is not None:
                if random_is_black == player_is_black:
                    if branch_threhsold <= 64 - now_obf.count("-"):
                        move = get_random_move(player, opponent)
                        assert re.fullmatch(r"[a-h][1-8]", move) is not None
                        break
                move = get_optimal_move(player, opponent)
                assert re.fullmatch(r"[a-h][1-8]", move) is not None
                break
            else:
                move = input()
            if move == "solve":
                move = get_optimal_move(player, opponent)
                assert re.fullmatch(r"[a-h][1-8]", move) is not None
                break
            if move == "random":
                move = get_random_move(player, opponent)
                assert re.fullmatch(r"[a-h][1-8]", move) is not None
                break
            if re.fullmatch(r"[a-hA-H][1-8]", move) is not None:
                if (bb_moves & (1 << reversi_misc.str2index(move.lower()))) == 0:
                    print(
                        "error: invalid input. The player cannot put a disc on the square."
                    )
                    continue
                break
            print(
                'error: invalid input. Please input a square (i.e., [a-hA-H][1-8]), "solve", or "random".'
            )
        square_index = reversi_misc.str2index(move.lower())
        flipped = reversi_misc.flip(square_index, player, opponent)
        assert flipped > 0
        next_player = opponent ^ flipped
        next_opponent = player ^ flipped ^ (1 << square_index)
        black, white = (
            (next_opponent, next_player)
            if player_is_black
            else (next_player, next_opponent)
        )
        player_is_black = not player_is_black
        gamerecord += move


if __name__ == "__main__":
    player_root()
