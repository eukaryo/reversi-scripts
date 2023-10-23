import datetime
import os
import re
import sys

# from reversi_misc import *
from reversi_solver_misc import read_empty50_tasklist_edax_knowledge

# tasklistのうち、ほかのknowledgeに書いてあるやつを探して、あれば自分のknowledgeに書き写したうえでtasklistから除去する。


def drop_duplicate_knowledge(problem_obf):
    # knowledgeファイルに完全一致する行が含まれている場合は除去する。

    if os.path.isfile(f"knowledge_{problem_obf[:64]}.csv") is False:
        return
    with open(f"knowledge_{problem_obf[:64]}.csv", "r", encoding="utf-8") as f:
        lines = [s.strip() for s in f.readlines()]
    outputs = set()
    with open(f"knowledge_{problem_obf[:64]}.csv", "w", encoding="utf-8") as f:
        for i in range(len(lines)):
            m = re.fullmatch(
                r"([-OX]{64}\s[OX];,-?[0-9]+,-?[0-9]+,-?[0-9]+,-?[0-9]+),-?[0-9]+",
                lines[i],
            )
            if m is None:
                assert i == 0
                f.write(lines[i] + "\n")
                continue
            if m.group(1) not in outputs:
                f.write(lines[i] + "\n")
                outputs.add(lines[i])


def read_all_obtained_knowledges():
    # すべてのknowledgeを読み込む。
    # 複数箇所にかかれている場合は、最も深く読まれているものだけを集める。
    # 最も深く読まれているものが複数ある場合は、答えの範囲が狭いものを優先する。
    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: read_all_obtained_knowledges"
    )
    obtained_knowledges = {}
    knowledge_filenames = [
        x
        for x in os.listdir()
        if "knowledge" in x and re.search(r"[-OX]{64}", x) is not None
    ]
    for i in range(len(knowledge_filenames)):
        filename = knowledge_filenames[i]
        if os.path.isfile(filename) is not True:
            continue
        with open(filename, "r", encoding="utf-8") as f:
            lines = [
                x.strip()
                for x in f.readlines()
                if re.search(r"[-OX]{64}\s[OX];", x.strip()) is not None
            ]
            for x in lines:
                m = re.fullmatch(
                    r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),-?[0-9]+",
                    x,
                )
                assert m is not None
                obf = m.group(1)
                if obf not in obtained_knowledges:
                    obtained_knowledges[obf] = x
                else:
                    depth = int(m.group(2))
                    accuracy = int(m.group(3))
                    current_strength = depth * 100 + accuracy
                    mm = re.fullmatch(
                        r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),-?[0-9]+",
                        obtained_knowledges[obf],
                    )
                    assert mm is not None
                    depth = int(mm.group(2))
                    accuracy = int(mm.group(3))
                    existing_strength = depth * 100 + accuracy
                    if current_strength > existing_strength:
                        obtained_knowledges[obf] = x
                    elif current_strength == existing_strength:
                        cur_lb = int(m.group(4))
                        cur_ub = int(m.group(5))
                        cur_range = cur_ub - cur_lb
                        exis_lb = int(mm.group(4))
                        exis_ub = int(mm.group(5))
                        exis_range = exis_ub - exis_lb
                        if (
                            cur_lb <= exis_lb
                            and exis_ub <= cur_ub
                            and exis_range < cur_range
                        ):
                            obtained_knowledges[obf] = x
    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: read_all_obtained_knowledges"
    )
    return obtained_knowledges


def solve_screening_tasklist(problem_obf, obtained_knowledges=None):
    assert os.path.isfile("empty50_tasklist_edax_knowledge.csv")
    tasklist = read_empty50_tasklist_edax_knowledge()
    assert problem_obf in tasklist
    assert os.path.isfile(f"{problem_obf[:64]}.csv")

    # tasklistを読み込む
    task_obfs = {}
    with open(f"{problem_obf[:64]}.csv", "r", encoding="utf-8") as f:
        task_lines = [x.strip() for x in f.readlines()]
        lines = [x for x in task_lines if re.search(r"[-OX]{64}\s[OX];", x) is not None]
        for x in lines:
            m = re.fullmatch(
                r"([-OX]{64}\s[OX];),(-?[0-9]{1,2}),(-?[0-9]{1,2}),(-?[0-9]{1,2})", x
            )
            assert m is not None
            obf = m.group(1)
            alpha = int(m.group(2))
            beta = int(m.group(3))
            estimated_score = int(m.group(4))
            task_obfs[obf] = (alpha, beta, estimated_score)

    if len(task_obfs) == 0:
        return False

    # すべてのknowledgeを読み込み、tasklist内の盤面に関するknowledgeを集める。
    # 複数箇所にかかれている場合は、最も深く読まれているものだけを集める。
    # 最も深く読まれているものが複数ある場合は、答えの範囲が狭いものを優先する。
    obtained_knowledges = {}
    knowledge_filenames = [
        x
        for x in os.listdir()
        if "knowledge" in x and re.search(r"[-OX]{64}", x) is not None
    ]
    for i in range(len(knowledge_filenames)):
        filename = knowledge_filenames[i]
        if os.path.isfile(filename) is not True:
            continue
        with open(filename, "r", encoding="utf-8") as f:
            lines = [
                x.strip()
                for x in f.readlines()
                if re.search(r"[-OX]{64}\s[OX];", x.strip()) is not None
            ]
            for x in lines:
                m = re.fullmatch(
                    r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),-?[0-9]+",
                    x,
                )
                assert m is not None
                obf = m.group(1)
                if obf in task_obfs:
                    if obf not in obtained_knowledges:
                        obtained_knowledges[obf] = x
                    else:
                        depth = int(m.group(2))
                        accuracy = int(m.group(3))
                        current_strength = depth * 100 + accuracy
                        mm = re.fullmatch(
                            r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),-?[0-9]+",
                            obtained_knowledges[obf],
                        )
                        assert mm is not None
                        depth = int(mm.group(2))
                        accuracy = int(mm.group(3))
                        existing_strength = depth * 100 + accuracy
                        if accuracy < 100:
                            continue  # 100%未満のknowledgeは無視する。p006の前のcollect imperfect knowledgesでカバーされるので。
                        if current_strength > existing_strength:
                            obtained_knowledges[obf] = x
                        elif current_strength == existing_strength:
                            cur_lb = int(m.group(4))
                            cur_ub = int(m.group(5))
                            cur_range = cur_ub - cur_lb
                            exis_lb = int(mm.group(4))
                            exis_ub = int(mm.group(5))
                            exis_range = exis_ub - exis_lb
                            if (
                                cur_lb <= exis_lb
                                and exis_ub <= cur_ub
                                and exis_range < cur_range
                            ):
                                obtained_knowledges[obf] = x

    found_knowledges = {}
    for obf, v in task_obfs.items():
        if obf in obtained_knowledges:
            found_knowledges[obf] = obtained_knowledges[obf]

    flag_imparfect_knowledge_loaded = False

    # knowledgeファイルに見つけた情報を追記する。knowledgeファイルに書かれたスコアの範囲と、taskのスコアの範囲が一致しない場合はフラグを立てる。
    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : info: len(found_knowledges) = {len(found_knowledges)}"
    )
    with open(f"knowledge_{problem_obf[:64]}.csv", "a") as f:
        for k, v in found_knowledges.items():
            f.write(f"{obtained_knowledges[k].strip()}\n")
            m = re.fullmatch(
                r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)",
                v,
            )
            assert m is not None
            knowledge_lb = int(m.group(4))
            knowledge_ub = int(m.group(5))
            task_alpha = task_obfs[k][0]
            task_beta = task_obfs[k][1]
            if task_beta < knowledge_lb or knowledge_ub < task_alpha:
                flag_imparfect_knowledge_loaded = True

    # tasklistのtaskのうち、完全読みのknowledgeが追記されたtaskを除去する。
    os.remove(f"{problem_obf[:64]}.csv")
    with open(f"{problem_obf[:64]}.csv", "w") as f:
        for line in task_lines:
            m = re.search(r"([-OX]{64})", line)
            if m is not None:
                obf = m.group(1) + " X;"
                if obf in found_knowledges:
                    mm = re.fullmatch(
                        r"([-OX]{64}\s[OX];),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)",
                        found_knowledges[obf],
                    )
                    assert mm is not None
                    depth = int(mm.group(2))
                    accuracy = int(mm.group(3))
                    if depth == 36 and accuracy == 100:
                        continue
            f.write(f"{line}\n")

    drop_duplicate_knowledge(problem_obf)

    return flag_imparfect_knowledge_loaded


if __name__ == "__main__":

    args = sys.argv
    if len(args) != 2:
        print("Error: len(args) == 2", file=sys.stderr)
        sys.exit(1)

    if re.fullmatch(r"[0-9]+", args[1]) is not None:
        problem_number = int(args[1]) - 1  # 1-originで来るので
        tasklist = read_empty50_tasklist_edax_knowledge()
        if problem_number < 0 or len(tasklist) <= problem_number:
            print("error: problem_number is invalid")
            sys.exit(1)
        problem_obf = tasklist[problem_number]
    elif re.fullmatch(r"[-OX]{64}", args[1]) is not None:
        assert args[1].count("-") == 50
        problem_obf = args[1] + " X;"
    else:
        print("error: args[1] is invalid.")
        sys.exit(1)

    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: solve_screening_tasklist({problem_obf})"
    )

    flag = solve_screening_tasklist(problem_obf)

    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : info: flag_imparfect_knowledge_loaded = {flag}"
    )
    print(
        f"{datetime.datetime.now().strftime(r'%Y/%m/%d %H:%M:%S')} : finish: solve_screening_tasklist({problem_obf})"
    )
