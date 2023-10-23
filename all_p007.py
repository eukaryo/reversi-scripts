import os
import re
import subprocess
import sys
import threading
from collections import deque
from datetime import datetime as dt

deque_task = deque()
threadLock = threading.Lock()
global_counter = 0
solver_counter = 0


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


def call_p007(obf):  # , output_max_abs = 64, ik = None):
    assert type(obf) is str
    assert re.fullmatch(r"[-OX]{64}\s[OX];", obf) is not None

    threadLock.acquire()
    print(
        f"{dt.now().strftime(r'%Y/%m/%d %H:%M:%S')} : start: call_p007: obf = \"{obf}\""
    )
    threadLock.release()

    cmd = [
        "./p007",
        "-o",
        obf,
        "-a",
        "-1",
        "-b",
        "1",
        "-k",
        f"knowledge_{obf[:64]}.csv",
        "-m",
        "64",
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
        print(f"Error: call_p007: failed to execute p007. obf = \"{obf}\"")
        sys.exit(0)
    finally:
        pass

    threadLock.acquire()
    print(
        f"{dt.now().strftime(r'%Y/%m/%d %H:%M:%S')} : finish: call_p007: obf = \"{obf}\""
    )
    threadLock.release()


def solve_py():

    global solver_counter, global_counter, threadLock, deque_task

    threadLock.acquire()
    solver_id = solver_counter
    solver_counter += 1
    threadLock.release()

    while True:

        threadLock.acquire()
        if len(deque_task) == 0:
            threadLock.release()
            return
        elif solver_id % 2 == 0:
            problem_obf = deque_task.popleft()
        else:
            problem_obf = deque_task.pop()
        global_counter += 1
        print(f"call start: {global_counter=}, {problem_obf=}")
        threadLock.release()

        call_p007(problem_obf)


if __name__ == "__main__":

    N_WORKERS = max(1, os.cpu_count())

    tasklist = read_empty50_tasklist_edax_knowledge()

    print(f"{len(tasklist)=}")
    tasklist = sorted(
        [(os.path.getsize(f"knowledge_{s[:64]}.csv"), s) for s in tasklist]
    )
    for s in tasklist:
        deque_task.append(s[1])

    threads_worker = []
    for _ in range(N_WORKERS):
        t = threading.Thread(target=solve_py)
        t.start()
        threads_worker.append(t)

    try:
        for t in threads_worker:
            t.join()
    except KeyboardInterrupt:
        print(
            f"{dt.now().strftime(r'%Y/%m/%d %H:%M:%S')} : info: KeyboardInterrupt detected. exit(1)"
        )
        exit(1)
