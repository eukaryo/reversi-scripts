import itertools
import os
import random
import re


def popcount(n):
    answer = 0
    while n != 0:
        answer += 1
        n = n & (n - 1)
    return answer


def bitscan_forward(n):
    assert n > 0
    answer = 0
    while n % 2 == 0:
        answer += 1
        n = n // 2
    return answer


def transpose(b):
    t = (b ^ (b >> 7)) & 0x00AA00AA00AA00AA
    b = (b ^ t ^ (t << 7)) & 0xFFFFFFFFFFFFFFFF
    t = (b ^ (b >> 14)) & 0x0000CCCC0000CCCC
    b = (b ^ t ^ (t << 14)) & 0xFFFFFFFFFFFFFFFF
    t = (b ^ (b >> 28)) & 0x00000000F0F0F0F0
    b = (b ^ t ^ (t << 28)) & 0xFFFFFFFFFFFFFFFF
    return b


def vertical_mirror(b):
    b = ((b >> 8) & 0x00FF00FF00FF00FF) | ((b << 8) & 0xFF00FF00FF00FF00)
    b = ((b >> 16) & 0x0000FFFF0000FFFF) | ((b << 16) & 0xFFFF0000FFFF0000)
    b = ((b >> 32) & 0x00000000FFFFFFFF) | ((b << 32) & 0xFFFFFFFF00000000)
    return b


def horizontal_mirror(b):
    b = ((b >> 1) & 0x5555555555555555) | ((b << 1) & 0xAAAAAAAAAAAAAAAA)
    b = ((b >> 2) & 0x3333333333333333) | ((b << 2) & 0xCCCCCCCCCCCCCCCC)
    b = ((b >> 4) & 0x0F0F0F0F0F0F0F0F) | ((b << 4) & 0xF0F0F0F0F0F0F0F0)
    return b


def board_symetry(s, player, opponent):

    if (s & 1) != 0:
        player = horizontal_mirror(player)
        opponent = horizontal_mirror(opponent)
    if (s & 2) != 0:
        player = vertical_mirror(player)
        opponent = vertical_mirror(opponent)
    if (s & 4) != 0:
        player = transpose(player)
        opponent = transpose(opponent)

    return (player, opponent)


def board_unique(player, opponent):
    return min([board_symetry(x, player, opponent) for x in range(8)])


def board_check(player, opponent):
    assert (player & opponent) == 0
    assert ((player | opponent) & ((2 ** 27) + (2 ** 28) + (2 ** 35) + (2 ** 36))) != 0


def flip(square, player, opponent):

    assert 0 <= square and square < 64

    direction = [-9, -8, -7, -1, 1, 7, 8, 9]
    edge = [
        0x01010101010101FF,
        0x00000000000000FF,
        0x80808080808080FF,
        0x0101010101010101,
        0x8080808080808080,
        0xFF01010101010101,
        0xFF00000000000000,
        0xFF80808080808080,
    ]

    flipped = 0
    for d in range(8):
        if ((1 << square) & edge[d]) == 0:
            f = 0
            x = square + direction[d]
            assert 0 <= x and x < 64
            while (opponent & (1 << x)) != 0 and ((1 << x) & edge[d]) == 0:
                assert 0 <= x and x < 64
                f |= 1 << x
                x += direction[d]
            assert 0 <= x and x < 64
            if (player & (1 << x)) != 0:
                flipped |= f
    return flipped


def get_some_moves(bb, mask, direction):
    # 1-stage Parallel Prefix (intermediate between kogge stone & sequential)
    # 6 << + 6 >> + 7 | + 10 &
    direction2 = direction + direction
    flip_l = mask & (bb << direction)
    flip_r = mask & (bb >> direction)
    flip_l |= mask & (flip_l << direction)
    flip_r |= mask & (flip_r >> direction)
    mask_l = mask & (mask << direction)
    mask_r = mask & (mask >> direction)
    flip_l |= mask_l & (flip_l << direction2)
    flip_r |= mask_r & (flip_r >> direction2)
    flip_l |= mask_l & (flip_l << direction2)
    flip_r |= mask_r & (flip_r >> direction2)
    return (flip_l << direction) | (flip_r >> direction)


def get_moves(player, opponent):
    mask = opponent & 0x7E7E7E7E7E7E7E7E
    a1 = get_some_moves(player, mask, 1)
    a2 = get_some_moves(player, opponent, 8)
    a3 = get_some_moves(player, mask, 7)
    a4 = get_some_moves(player, mask, 9)
    empty_bb = (player | opponent) ^ 0xFFFFFFFFFFFFFFFF
    return (a1 | a2 | a3 | a4) & empty_bb


def str2index(s):
    if s[0] == "a":
        answer = 0
    elif s[0] == "b":
        answer = 1
    elif s[0] == "c":
        answer = 2
    elif s[0] == "d":
        answer = 3
    elif s[0] == "e":
        answer = 4
    elif s[0] == "f":
        answer = 5
    elif s[0] == "g":
        answer = 6
    elif s[0] == "h":
        answer = 7
    else:
        assert False
    if s[1] == "1":
        answer += 8 * 0
    elif s[1] == "2":
        answer += 8 * 1
    elif s[1] == "3":
        answer += 8 * 2
    elif s[1] == "4":
        answer += 8 * 3
    elif s[1] == "5":
        answer += 8 * 4
    elif s[1] == "6":
        answer += 8 * 5
    elif s[1] == "7":
        answer += 8 * 6
    elif s[1] == "8":
        answer += 8 * 7
    else:
        assert False
    return answer


def index2str(index):
    assert 0 <= index and index < 64
    return "abcdefgh"[index % 8] + str((index // 8) + 1)


def unittest_index_str():
    print("start: unittest_index_str")
    for i in range(64):
        assert str2index(index2str(i)) == i
        assert index2str(str2index(index2str(i))) == index2str(i)
    print("clear: unittest_index_str")


def gamerecord_symmetry(s, gamerecord):
    answer = ""
    for i in range(0, len(gamerecord), 2):
        move = gamerecord[i : i + 2]
        assert re.fullmatch(r"[a-h][1-8]", move) is not None
        index = str2index(move)
        bb = 1 << index
        answer += index2str(bitscan_forward(board_symetry(s, bb, 0)[0]))
    return answer


def regularize_kihu(gamerecord):
    for i in range(8):
        answer = gamerecord_symmetry(i, gamerecord)
        if answer[:2] == "f5":
            return answer
    assert False


def bitboards_to_obf(player, opponent, player_is_black=True):
    assert (player & opponent) == 0
    answer = ""
    for i in range(64):
        if (player & (1 << i)) != 0:
            answer += "X" if player_is_black else "O"
        elif (opponent & (1 << i)) != 0:
            answer += "O" if player_is_black else "X"
        else:
            answer += "-"
    return answer + (" X;" if player_is_black else " O;")


def obf_to_bitboards(s):
    assert re.fullmatch(r"[-OX]{64}\s[OX];", s) is not None
    player = 0
    opponent = 0
    for i in range(64):
        if s[i] == s[65]:
            player += 1 << i
        elif s[i] != "-":
            opponent += 1 << i
    return (player, opponent)


def obf_unique(obf):
    assert re.fullmatch(r"[-OX]{64}\s[OX];", obf) is not None
    player = 0
    opponent = 0
    for i in range(64):
        if obf[i] == obf[65]:
            player += 2 ** i
        elif obf[i] != "-":
            opponent += 2 ** i
    board_check(player, opponent)
    x = board_unique(player, opponent)
    return bitboards_to_obf(x[0], x[1])


def uinttest_obf_bitboard_conversion(seed, num_iter):
    random.seed(seed)
    print("start: uinttest_obf_bitboard_conversion")
    for i in range(num_iter):
        player = 0
        opponent = 0
        for j in range(64):
            x = random.randint(0, 2)
            if x == 0:
                player += 1 << j
            elif x == 1:
                opponent += 1 << j
        obf = bitboards_to_obf(player, opponent)
        (p, o) = obf_to_bitboards(obf)
        assert p == player and o == opponent
        obf2 = bitboards_to_obf(p, o)
        assert obf == obf2
    print("clear: uinttest_obf_bitboard_conversion")


def gamerecord_to_obf(gamerecord: str) -> str:
    assert re.fullmatch(r"([a-hA-H][1-8]){0,60}", gamerecord) is not None
    black, white = obf_to_bitboards(initial_position("cross"))
    player_is_black = True
    moves = [gamerecord[i : i + 2].lower() for i in range(0, len(gamerecord), 2)]
    for move in moves:
        player, opponent = (black, white) if player_is_black else (white, black)
        bb_moves = get_moves(player, opponent)
        if bb_moves == 0:
            if get_moves(opponent, player) == 0:
                break
            player_is_black = not player_is_black
            player, opponent = (black, white) if player_is_black else (white, black)
            bb_moves = get_moves(player, opponent)
        square_index = str2index(move)
        assert (bb_moves & (1 << square_index)) > 0
        flipped = flip(square_index, player, opponent)
        assert flipped > 0
        next_player = opponent ^ flipped
        next_opponent = player ^ flipped ^ (1 << square_index)
        black, white = (
            (next_opponent, next_player)
            if player_is_black
            else (next_player, next_opponent)
        )
        player_is_black = not player_is_black
    return bitboards_to_obf(black, white, player_is_black)


def ComputeFinalScore(player, opponent):

    # 引数の盤面が即詰みだと仮定し、最終スコアを返す。

    n_discs_p = popcount(player)
    n_discs_o = popcount(opponent)

    score = n_discs_p - n_discs_o

    # 空白マスが残っている状態で両者とも打つ場所が無い場合は試合終了だが、
    # そのとき引き分けでないならば、空白マスは勝者のポイントに加算されるというルールがある。
    if score < 0:
        score -= 64 - n_discs_p - n_discs_o
    elif score > 0:
        score += 64 - n_discs_p - n_discs_o

    return score


def read_table(query, filename="all_result_abtree_encoded_sorted_unique.csv"):
    assert len(query) == 16

    def _verify_and_check(line):
        assert len(line) == 18
        assert line[16] in "01234"
        assert line[17] == "\n"
        return query == line[:16]

    f = open(filename, "r")
    firstline = f.readline()
    if _verify_and_check(firstline):
        return firstline[16]

    filesize = f.seek(0, os.SEEK_END)
    assert filesize % len(firstline) == 0

    f.seek(filesize - 18, os.SEEK_SET)
    finalline = f.readline()
    if _verify_and_check(finalline):
        return finalline[16]

    lb, ub = 0, filesize // len(firstline)
    while lb + 1 < ub:
        mid = (lb + ub) // 2
        f.seek(mid * len(firstline), os.SEEK_SET)
        midline = f.readline()
        if _verify_and_check(midline):
            return midline[16]
        if query < midline[:16]:
            ub = mid
        else:
            lb = mid
    return None


def make_obf64_to_base81_dict():
    codebook = r"56789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#_"
    assert len(codebook) == 81
    assert len(set(list(codebook))) == 81
    codes = sorted(["".join(x) for x in itertools.product(["-", "O", "X"], repeat=4)])
    assert len(codes) == 81
    return dict(zip(codes, list(codebook)))


obf64_to_base81_dict = make_obf64_to_base81_dict()
base81_to_obf64_dict = {v: k for k, v in obf64_to_base81_dict.items()}


def obf_to_base81encoding(obf):
    assert re.fullmatch(r"[-OX]{64}\s[OX];", obf) is not None
    if obf[65] == "O":
        obf.translate(str.maketrans({"O": "X", "X": "O"}))

    answer = "".join([obf64_to_base81_dict[obf[i : i + 4]] for i in range(0, 64, 4)])
    assert len(answer) == 16
    return answer


def base81encodeing_to_obf(code):
    answer = "".join([base81_to_obf64_dict[code[i]] for i in range(0, 16)])
    assert re.fullmatch(r"[-OX]{64}", answer) is not None
    return answer + " X;"


def unittest_obf_base81encoding_conversion(seed, num_iter):
    random.seed(seed)
    print("start: unittest_obf_base81encoding_conversion")
    for _ in range(num_iter):
        obf = ""
        for _ in range(64):
            x = random.randint(0, 2)
            if x == 0:
                obf += "O"
            elif x == 1:
                obf += "X"
            elif x == 2:
                obf += "-"
            else:
                assert False
        obf += " X;"
        code = obf_to_base81encoding(obf)
        obf2 = base81encodeing_to_obf(code)
        code2 = obf_to_base81encoding(obf2)
        assert obf == obf2
        assert code == code2
    print("clear: unittest_obf_base81encoding_conversion")


def initial_position(kind):
    if kind == "cross":
        black = (1 << str2index("e4")) + (1 << str2index("d5"))
        white = (1 << str2index("d4")) + (1 << str2index("e5"))
    elif kind == "parallel":
        black = (1 << str2index("d5")) + (1 << str2index("e5"))
        white = (1 << str2index("d4")) + (1 << str2index("e4"))
    else:
        assert False
    return (black, white)


if __name__ == "__main__":
    uinttest_obf_bitboard_conversion(12345, 10000)
    unittest_index_str()
    unittest_obf_base81encoding_conversion(12345, 10000)
