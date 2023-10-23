import os
import re
import subprocess

import reversi_solver_misc

# このスクリプトは、2587個すべてのsubproblemsを解いた方法を模式的に示すためだけのものです。
# このスクリプトを最後まで走らせれば実際に解くことができますが、非常に時間がかかります。
# このスクリプトを参考にして、手元のクラスタなどで並列化するコードを書いて解くのがよいでしょう。
if __name__ == "__main__":
    n_tasks = max(1, os.cpu_count())
    tasklist = reversi_solver_misc.read_empty50_tasklist_edax_knowledge()
    assert len(tasklist) == 2587
    while True:
        solved_flag = True
        subprocess.run(["python", "all_p006.py"])
        for x in tasklist:
            assert re.fullmatch(r"[-OX]{64}\s[OX];", x)
            if os.path.isfile(f"{x[:64]}.csv"):
                with open(f"{x[:64]}.csv", "r") as f:
                    lines = [
                        s.strip()
                        for s in f.readlines()
                        if re.search(r"[-OX]{64}\s[OX];", s.strip()) is not None
                    ]
                if len(lines) > 0:
                    solved_flag = False
                    subprocess.run(["python", "solve_one_e50_subproblem.py", x[:64], n_tasks])
        if solved_flag:
            break
    exit(0)
