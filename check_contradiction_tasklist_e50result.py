import os
import re


def check():

    assert os.path.isfile("empty50_tasklist_edax_knowledge.csv")

    tasklist = []
    with open("empty50_tasklist_edax_knowledge.csv", "r", encoding="utf-8") as f:
        lines = [
            x.strip()
            for x in f.readlines()
            if re.search(r"[-OX]{64}\s[OX];", x.strip()) is not None
        ]
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
            assert -64 <= score and score <= 64 and abs(score) % 2 == 0
            tasklist.append({"obf": obf, "score": score, "alpha": alpha, "beta": beta})

    num_solved = 0
    for task in tasklist:
        obf = task["obf"]
        assert os.path.isfile(f"result_e50{obf[:64]}.csv")
        with open(f"result_e50{obf[:64]}.csv", "r") as f:
            lines = [x.strip() for x in f.readlines()]
            assert len(lines) == 2
            assert lines[0] == "obf,is_exact,score_lowerbound,score_upperbound"
            m = re.fullmatch(
                r"([-OX]{64}\s[OX];),([01]),(-?[0-9]{1,2}),(-?[0-9]{1,2})", lines[1]
            )
            assert m is not None
            assert obf == m.group(1)
            num_solved += int(m.group(2))
            score_tasklist = task["score"]
            alpha_tasklist = task["alpha"]
            beta_tasklist = task["beta"]
            alpha_result = int(m.group(3))
            beta_result = int(m.group(4))
            assert alpha_result <= beta_result
            if score_tasklist <= alpha_tasklist:
                assert beta_result <= alpha_tasklist
            elif beta_tasklist <= score_tasklist:
                assert beta_tasklist <= alpha_result
            elif alpha_tasklist < score_tasklist and score_tasklist < beta_tasklist:
                assert alpha_tasklist < alpha_result and beta_result < beta_tasklist

    print("check_contradiction_tasklist_e50result.py: check() is finished.")
    print("clear: no contradiction was found.")
    print(f"info: {len(tasklist)} tasks are checked.")
    print(
        f"info: {num_solved} task(s) is/are solved. "
        "(this information is valid only immediately after executing all_p006.py)"
    )


if __name__ == "__main__":
    check()
