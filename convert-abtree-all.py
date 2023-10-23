import itertools
import os
import re
import subprocess
import sys

import reversi_misc


def get_all_filenames_abtree():
    answer = []
    for s in os.listdir():
        if os.path.isfile(s):
            if re.fullmatch(r"result_[-OX]{64}_abtree.csv", s):
                answer.append(s)
            elif s == "result_e50_opening_book.csv":
                answer.append(s)
    return answer


def get_single_abtree_encoded(filename):
    lines = []
    with open(filename, "r") as f:
        for line in f:
            m = re.search(r"([-OX]{64}\s[OX];)", line)
            if m is None:
                continue
            obf64_encoded = reversi_misc.obf_to_base81encoding(m.group(1))
            if re.search(r";,[1-9][0-9]?,[1-9][0-9]?", line):
                lines.append(f"{obf64_encoded}4")
            elif ";,0,64" in line:
                lines.append(f"{obf64_encoded}3")
            elif ";,0,0" in line:
                lines.append(f"{obf64_encoded}2")
            elif ";,-64,0" in line:
                lines.append(f"{obf64_encoded}1")
            elif re.search(r";,-[1-9][0-9]?,-[1-9][0-9]?", line):
                lines.append(f"{obf64_encoded}0")
            elif ";,-64,64" in line:
                continue
            else:
                print(f"error {line}")
                sys.exit(1)
    return lines


if __name__ == "__main__":

    filenames = get_all_filenames_abtree()
    assert len(filenames) == 2588
    with open("all_result_abtree_encoded_sorted_unique.csv", "w") as f:
        for i, filename in enumerate(filenames):
            # print(f"processing {filename}")
            print(f"processing {filename} : {i+1} / {len(filenames)}", flush=True)
            for line in get_single_abtree_encoded(filename):
                f.write(line + "\n")
    print("start: sort and uniquefy the output file")
    subprocess.run(
        [
            "sort",
            "-u",
            "all_result_abtree_encoded_sorted_unique.csv",
            "-o",
            "all_result_abtree_encoded_sorted_unique.csv",
        ]
    )
    print("finish: sort and uniquefy the output file")
