#!/bin/sh
git clone https://github.com/eukaryo/edax-reversi-AVX-v446mod2.git -b v446mod2
cd edax-reversi-AVX-v446mod2
mkdir bin
cd src
make build ARCH=x64-modern COMP=gcc
cd ../
cp bin/lEdax-x64-modern ../Edax_mod2
bunzip2 eval.dat.bz2
mkdir ../data
mv eval.dat ../data
cd ../
rm -rf edax-reversi-AVX-v446mod2
python3 dat_to_json.py
bunzip2 -k opening_book_freq.csv.bz2
