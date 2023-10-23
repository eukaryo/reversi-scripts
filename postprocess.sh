#!/bin/sh
make
bunzip2 -k opening_book_freq.csv.bz2
python3 all_p006.py
python3 check_contradiction_tasklist_e50result.py
python3 make_50_book.py
python3 all_p007.py
