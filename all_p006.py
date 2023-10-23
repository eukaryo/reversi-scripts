import os
import re
import threading
from collections import deque
from datetime import datetime as dt

import check_contradiction_tasklist_e50result
import reversi_solver_misc

deque_task = deque()
threadLock = threading.Lock()
global_counter = 0
solver_counter = 0


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
        threadLock.release()

        if os.path.isfile(f"result_e50{problem_obf[:64]}.csv"):
            with open(f"result_e50{problem_obf[:64]}.csv", "r") as f:
                x = "".join([s.strip() for s in f.readlines()])
                m = re.search(r"[-OX]{64}\s[OX];,1,", x)
                if m is not None:
                    threadLock.acquire()
                    global_counter += 1
                    print(
                        f"{dt.now().strftime(r'%Y/%m/%d %H:%M:%S')}: info: {global_counter=}, solved: {problem_obf=}"
                    )
                    threadLock.release()
                    continue
        if os.path.isfile(f"{problem_obf[:64]}.csv"):
            os.remove(f"{problem_obf[:64]}.csv")

        threadLock.acquire()
        global_counter += 1
        print(
            f"{dt.now().strftime(r'%Y/%m/%d %H:%M:%S')} : call start: {global_counter=}, {problem_obf=}"
        )
        threadLock.release()

        reversi_solver_misc.call_p006(problem_obf, ik=[])


def ckeck_all_results_if_solved():
    solved, unsolved = 0, 0
    for problem_obf in reversi_solver_misc.read_empty50_tasklist_edax_knowledge():
        if os.path.isfile(f"result_e50{problem_obf[:64]}.csv"):
            with open(f"result_e50{problem_obf[:64]}.csv", "r") as f:
                x = "".join([s.strip() for s in f.readlines()])
                m = re.search(r"[-OX]{64}\s[OX];,1,", x)
                if m is not None:
                    solved += 1
                else:
                    unsolved += 1
    print(f"{solved=}, {unsolved=}")
    if unsolved == 0:
        check_contradiction_tasklist_e50result.check()


if __name__ == "__main__":

    N_WORKERS = max(1, os.cpu_count())
    print(f"{dt.now().strftime(r'%Y/%m/%d %H:%M:%S')} : info: {N_WORKERS=}")

    tasklist = reversi_solver_misc.read_empty50_tasklist_edax_knowledge()

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

    ckeck_all_results_if_solved()
