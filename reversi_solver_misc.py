import datetime
import os
import re
import shutil
import subprocess
import sys

import reversi_misc


def recognize_problem(lines):
    # ファイルの最初に書いてある盤面を読み取ってビットボードにする。
    for i in range(len(lines)):
        if "A B C D E F G H" in lines[i]:
            if "A B C D E F G H" not in lines[i + 9]:
                continue
            player = "*"
            opponent = "O"
            if "O to move" in lines[i + 2]:
                player = "O"
                opponent = "*"
            elif "* to move" not in lines[i + 2]:
                assert False
            pbb = 0
            obb = 0
            for j in range(1, 9):
                m = re.match(r"[1-8](\s[-O\*\.]){8}\s[1-8]", lines[i + j])
                assert m is not None
                for k in range(8):
                    if lines[i + j][2 + 2 * k] == player:
                        pbb += 2 ** (j * 8 - 8 + k)
                    elif lines[i + j][2 + 2 * k] == opponent:
                        obb += 2 ** (j * 8 - 8 + k)
            assert 0 <= pbb and pbb < (2 ** 64)
            assert 0 <= obb and obb < (2 ** 64)
            assert (pbb & obb) == 0
            assert ((pbb | obb) & (2 ** 27)) != 0
            assert ((pbb | obb) & (2 ** 28)) != 0
            assert ((pbb | obb) & (2 ** 35)) != 0
            assert ((pbb | obb) & (2 ** 36)) != 0
            return (pbb, obb)
    return None


def get_result_filenames(target_directory):
    a = os.listdir(target_directory)
    b = [f for f in a if os.path.isfile(os.path.join(target_directory, f))]
    return [os.path.join(target_directory, f) for f in b]


def read_obf_file(filename):
    with open(filename, "r") as f:
        lines = [
            s.strip()
            for s in f.readlines()
            if re.fullmatch(r"[-OX]{64}\s[OX];", s.strip()) is not None
        ]
        return lines
    print("Error: problem file does not exist", file=sys.stderr)
    sys.exit(1)


def read_result_file(filename):
    try:
        with open(filename, "r", encoding="utf-8") as f:
            lines = [s.strip() for s in f.readlines()]
            return lines
    except OSError:
        try:
            with open(filename, "r", encoding="utf-8_sig") as f:
                lines = [s.strip() for s in f.readlines()]
                return lines
        except OSError:
            return []
    return []


def timestr2int(s):
    # "1:23.456" みたいな消費時間の文字列を受け取って秒になおす。
    m = re.fullmatch(r"([0-9]+):([0-9]+)\.([0-9]+)", s)
    if m is not None:
        return max(1, int(m.group(1)) * 60 + int(m.group(2)))
    m = re.fullmatch(r"([0-9]+):([0-9]+):([0-9]+)\.([0-9]+)", s)
    if m is not None:
        return max(
            1, int(m.group(1)) * 60 * 60 + int(m.group(2)) * 60 + int(m.group(3))
        )
    m = re.fullmatch(r"([0-9]+):([0-9]+):([0-9]+):([0-9]+)\.([0-9]+)", s)
    if m is not None:
        return max(
            1,
            int(m.group(1)) * 60 * 60 * 24
            + int(m.group(2)) * 60 * 60
            + int(m.group(3)) * 60
            + int(m.group(4)),
        )
    print("Error: we could not interpret timestr.")
    return -1


def scorestr2range(s):
    m = re.fullmatch(r"^[><]?[-+][0-9]{2}$", s)
    assert m is not None
    if s[0] == "<":
        return (-64, int(s[1:]))
    if s[0] == ">":
        return (int(s[1:]), 64)
    score = int(s)
    return (score, score)


def compute_one_verbose2_problem(lines):
    bb = recognize_problem(lines)
    assert bb is not None
    obf = reversi_misc.obf_unique(reversi_misc.bitboards_to_obf(bb[0], bb[1]))
    num_empty = obf.count("-")

    lines.reverse()

    for x in lines:
        m = re.search(
            r"([0-9]+)\s+([><]?[-+][0-9]{2})\s+([0-9.:]+)\s+([0-9]+)\s+([0-9]*)\s+(([a-hA-H][1-8]\s?)+)",
            x,
        )
        if m is not None:
            depth = int(m.group(1))
            assert depth == num_empty
            score_range = scorestr2range(m.group(2))
            nodes = int(m.group(4))
            principal_variation = m.group(6).lower().strip().split(" ")
            return {
                "perfect": True,
                "depth": depth,
                "accuracy": 100,
                "obf": obf,
                "nodes": nodes,
                "score_lowerbound": score_range[0],
                "score_upperbound": score_range[1],
                "principal_variation": principal_variation,
            }

        m = re.search(
            r"([0-9]+)@([0-9]+)%\s+([-+][0-9]{2})\s+([0-9.:]+)\s+([0-9]+)\s+([0-9]*)\s+(([a-hA-H][1-8]\s?)+)",
            x,
        )
        if m is not None:
            depth = int(m.group(1))
            accuracy = int(m.group(2))
            score = int(m.group(3))
            nodes = int(m.group(5))
            principal_variation = m.group(7).lower().strip().split(" ")
            assert accuracy < 100
            return {
                "perfect": False,
                "depth": depth,
                "accuracy": accuracy,
                "obf": obf,
                "nodes": nodes,
                "score_lowerbound": score,
                "score_upperbound": score,
                "principal_variation": principal_variation,
            }

    return None


def read_empty50_tasklist_edax_knowledge():

    if os.path.isfile("empty50_tasklist_edax_knowledge.csv") is False:
        print("error: empty50_tasklist_edax_knowledge.csv not found.")
        sys.exit(0)

    with open("empty50_tasklist_edax_knowledge.csv", "r", encoding="utf-8") as f:

        lines = [
            x.strip()
            for x in f.readlines()
            if re.search(r"[-OX]{64}\s[OX];", x.strip()) is not None
        ]

        answer = []
        for x in lines:
            m = re.fullmatch(
                r"([-OX]{64}\s[OX];),([0-9]+),(-?[0-9]{1,2}),(-?[0-9]{1,2}),(-?[0-9]{1,2})",
                x,
            )
            obf = m.group(1)
            count = int(m.group(2))
            score = int(m.group(3))
            alpha = int(m.group(4))
            beta = int(m.group(5))
            n_empties = obf.count("-")
            assert n_empties == 50
            assert 0 <= count
            assert -64 <= alpha and alpha <= beta and beta <= 64
            assert -64 <= score and score <= 64 and score == (score // 2) * 2
            answer.append((count if count >= 5 else 0, -abs(score), obf))
        return [x[2] for x in sorted(answer, reverse=True)]


def write_obf_file(obffilename, obftext):
    with open(obffilename, "w") as f:
        for x in obftext:
            f.write(x + "\n")
        return True
    return False


def is_solved_problem(problem_obf):
    if not os.path.isfile(f"result_e50{problem_obf[:64]}.csv"):
        return False
    with open(f"result_e50{problem_obf[:64]}.csv", "r") as f:
        text = "".join(f.readlines())
    return ";,1" in text


def collect_imperfect_knowledges():
    result = []
    knowledge_filenames = [
        x
        for x in os.listdir()
        if "knowledge" in x and re.search(r"[-OX]{64}", x) is not None
    ]
    for i in range(len(knowledge_filenames)):
        filename = knowledge_filenames[i]
        with open(filename, "r", encoding="utf-8") as f:
            lines = [
                x.strip()
                for x in f.readlines()
                if re.search(r"[-OX]{64}\s[OX];", x.strip()) is not None
            ]
            for x in lines:
                # obf,depth,accuracy,score_lowerbound,score_upperbound,nodes
                m = re.fullmatch(
                    r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)",
                    x,
                )
                assert m is not None
                depth = int(m.group(2))
                accuracy = int(m.group(3))
                strength = depth * 100 + accuracy
                if strength != 3700:
                    result.append(x)
    return result


def call_p006(obf, output_max_abs=64, ik=None):
    assert type(obf) is str
    assert re.fullmatch(r"[-OX]{64}\s[OX];", obf) is not None
    assert type(output_max_abs) is int
    assert 0 <= output_max_abs and output_max_abs <= 64

    if os.path.isfile(f"__kn0wledge_{obf[:64]}.csv"):
        os.remove(f"__kn0wledge_{obf[:64]}.csv")

    if ik == []:
        print(
            f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: call_p006: obf = \"{obf}\""
        )
        cmd = [
            "./p006",
            "-o",
            obf,
            "-a",
            "-1",
            "-b",
            "1",
            "-k",
            f"knowledge_{obf[:64]}.csv",
            "-m",
            str(output_max_abs),
        ]
    else:
        print(
            f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: call_p006: making knowledge, obf = \"{obf}\""
        )
        if ik is None:
            ik = collect_imperfect_knowledges()
        if os.path.isfile(f"knowledge_{obf[:64]}.csv"):
            shutil.copy(f"knowledge_{obf[:64]}.csv", f"__kn0wledge_{obf[:64]}.csv")
        with open(f"__kn0wledge_{obf[:64]}.csv", "a") as f:
            for x in ik:
                f.write(x + "\n")
        print(
            f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: call_p006: obf = \"{obf}\""
        )
        cmd = [
            "./p006",
            "-o",
            obf,
            "-a",
            "-1",
            "-b",
            "1",
            "-k",
            f"__kn0wledge_{obf[:64]}.csv",
            "-m",
            str(output_max_abs),
        ]

    try:
        subprocess.run(
            cmd,
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            universal_newlines=True,
        )

    except subprocess.CalledProcessError:
        print("Error: call_p006: failed to execute p006")
        if os.path.isfile(f"__kn0wledge_{obf[:64]}.csv"):
            os.remove(f"__kn0wledge_{obf[:64]}.csv")
        sys.exit(0)
    finally:
        if os.path.isfile(f"__kn0wledge_{obf[:64]}.csv"):
            os.remove(f"__kn0wledge_{obf[:64]}.csv")

    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : finish: call_p006: obf = \"{obf}\""
    )
    if os.path.isfile(f"__kn0wledge_{obf[:64]}.csv"):
        os.remove(f"__kn0wledge_{obf[:64]}.csv")

    if os.path.isfile(f"{obf[:64]}.csv"):
        with open(f"{obf[:64]}.csv", "r") as f:
            for line in f:
                if "obf,alpha,beta,estimated_score" not in line:
                    return
        os.remove(f"{obf[:64]}.csv")


def add_knowledge(filename, knowledge_dict):
    with open(filename, "a") as f:
        f.write(
            f"{knowledge_dict['obf']},{knowledge_dict['depth']},{knowledge_dict['accuracy']},{knowledge_dict['score_lowerbound']},{knowledge_dict['score_upperbound']},{knowledge_dict['nodes']}\n"
        )


def read_knowledge_file(filename):
    answer = {}
    if os.path.isfile(filename) is False:
        return ()
    with open(filename, "r") as f:
        lines = [s.strip() for s in f.readlines()]
        for x in lines:
            m = re.match(
                r"([-OX]{64}\s[OX];),([0-9]{1,2}),([0-9]{2,3}),(-?[0-9]{1,2}),(-?[0-9]{1,2}),(-?[0-9]+)",
                x,
            )
            if m is None:
                if x != "obf,depth,accuracy,score_lowerbound,score_upperbound,nodes":
                    print(f"warning: read_knowledge_file: {x}")
                continue
            obf = reversi_misc.obf_unique(m.group(1))
            depth = int(m.group(2))
            accuracy = int(m.group(3))
            score_lowerbound = int(m.group(4))
            score_upperbound = int(m.group(5))
            nodes = int(m.group(6))
            if obf not in answer:
                answer[obf] = {
                    "depth": depth,
                    "accuracy": accuracy,
                    "score_lowerbound": score_lowerbound,
                    "score_upperbound": score_upperbound,
                    "nodes": nodes,
                }
            elif answer[obf]["depth"] < depth:
                answer[obf] = {
                    "depth": depth,
                    "accuracy": accuracy,
                    "score_lowerbound": score_lowerbound,
                    "score_upperbound": score_upperbound,
                    "nodes": nodes,
                }
            elif answer[obf]["depth"] == depth and answer[obf]["accuracy"] < accuracy:
                answer[obf] = {
                    "depth": depth,
                    "accuracy": accuracy,
                    "score_lowerbound": score_lowerbound,
                    "score_upperbound": score_upperbound,
                    "nodes": nodes,
                }
            elif (
                answer[obf]["depth"] == depth
                and answer[obf]["accuracy"] == accuracy
                and answer[obf]["nodes"] < 0
                and nodes > 0
            ):
                answer[obf] = {
                    "depth": depth,
                    "accuracy": accuracy,
                    "score_lowerbound": score_lowerbound,
                    "score_upperbound": score_upperbound,
                    "nodes": nodes,
                }
    return answer


def read_knowledge_file_setonly(filename):
    answer = set()
    with open(filename, "r") as f:
        lines = [s.strip() for s in f.readlines()]
        for x in lines:
            m = re.match(
                r"([-OX]{64}\s[OX];),([0-9]{1,2}),([0-9]{2,3}),(-?[0-9]{1,2}),(-?[0-9]{1,2}),(-?[0-9]+)",
                x,
            )
            if m is None:
                if x != "obf,depth,accuracy,score_lowerbound,score_upperbound,nodes":
                    print(f"warning: read_knowledge_file: {x}")
                continue
            obf = reversi_misc.obf_unique(m.group(1))
            answer.add(obf)
    return answer


def clear_problem_obf(e50obf):
    for x in os.listdir("./"):
        if os.path.isfile(x):
            filename = e50obf[:64] + r"_[0-9]+%_[-OX]{64}\.obf"
            if re.fullmatch(filename, x) is not None:
                os.remove(x)


if __name__ == "__main__":
    pass
