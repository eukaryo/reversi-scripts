# reversi-scripts

The files containing the analysis results can be downloaded from https://doi.org/10.6084/m9.figshare.24420619 . Inside, there are 2587 csv files named like knowledge\_[-OX]{64}\\.csv . Please place them in the same directory as the files in this repository.

<!--
解析結果のファイルは https://doi.org/10.6084/m9.figshare.24420619 からダウンロードできます。そのなかには knowledge\_[-OX]{64}\\.csv といったファイル名のcsvファイルが2587個あります。それらを本リポジトリのファイルと同じディレクトリに入れて下さい。
-->

## About knowledge\_[-OX]{64}\\.csv

There are 2587 files with names matching the regular expression knowledge\_[-OX]{64}\\.csv . Each file represents computation results. Each row is a result computed by Edax. The columns, from left to right, are: the OBF representation of the position with 36 empty squares, depth fed to Edax, strength fed to Edax (100 means complete analysis), upper score bound, lower score bound, and the number of positions Edax explored.

The upper and lower score bounds represent the true theoretical values when the depth is 36 and the strength is 100. If the strength is not 100, the same value is entered, indicating a predicted value. The number of positions Edax explored is positive when the command-line option given to Edax looks like:
```
$ ./Edax_mod2 -solve hoge.obf -level 60 -n 1 -hash-table-size 23 -verbose 2 -alpha -3 -beta 3 -width 200
```
Running with these options should return the exact same number of explored positions (reproducibility was prioritized). A negative value means different command-line options were used, for instance, parallelized or different alpha and beta values.

<!--
ファイル名が正規表現で knowledge\_[-OX]{64}\\.csv となっている2587個のファイルが計算結果たちです。
各rowがEdaxに読ませた結果です。columnsは左から、読ませた36マス空き局面のOBF表現、読ませた深さ、読ませた強さ(100だと完全読み)、スコアの上界、下界、Edaxが出力した探索局面数　です。

スコアの上界、下界に関しては、深さ36で強さ100のときには真の理論値に対する上界と下界です。強さ100でない場合は同じ値が入っていて、予測値を意味します。
Edaxが出力した探索局面数に関しては、正の値のときはEdaxに与えたコマンドラインオプションが
```
$ ./Edax_mod2 -solve hoge.obf -level 60 -n 1 -hash-table-size 23 -verbose 2 -alpha -3 -beta 3 -width 200
```
といった感じです。このオプションで投げ直せば全く同じ探索局面数を返すはずです。（再現性を重視しました）
負の値はこれ以外のコマンドラインオプションを使って計算したことを意味します。並列化したり、alphaとbetaの値が異なっていたりします。
-->

## Reproducing the post-processing

The following has been tested with python 3.8.10.

### 0

First, execute:
```
$ make
$ bunzip2 -k opening_book_freq.csv.bz2
$ sh prep-edax-and-eval.sh
```
This will produce four executable files: p006, p007, solve33, and p008\_manyeval.

### 1

p006 takes a 50-empty square position (p) and a knowledge file (k) as arguments (for details, please read Source.cpp). It checks if the position p can be weakly solved using the knowledge from file k. If it cannot, it outputs a set of "predictions" that would be sufficient for weak resolution. Even if it can be weakly solved, it still outputs a file, but it's just a CSV file with a header, indicating that the set of "predictions" is empty. Here, a "prediction" is a row in the CSV file, consisting of the OBF format of the position with 36 empty squares, lower score bound, upper score bound, and a predicted score. The prediction suggests that the game-theoretic value of that position with 36 empty squares likely falls within that range. If all "predictions" are positively resolved, then the original 50-empty square position is considered resolved. The final predicted score isn't directly related to the "prediction" itself but is provided for reference in case one wants to solve more challenging positions first.

In addition, p006 outputs files named like result\_e50[-OX]{64}\\.csv . If the proof is complete, this file contains the upper and lower game-theoretic value bounds as proof results.

Running all_p006.py will execute p006 in parallel for all 2587 positions.

<!--
p006は、50マス空き局面pとknowledgeファイルkなど（詳しくはSource.cppを読んで下さい）を引数に取り、kの知識でpを弱解決できるかどうか調べて、できないならば弱解決に十分であるような"予想"の集合をファイル出力します。（弱解決できる場合もファイル出力自体はしますが、見出し行だけのcsvファイルです。"予想"の集合が空集合だということです）ここで"予想"と言っているのは、具体的にはcsvファイルのrowでして、36マス空き局面のOBF形式、スコアの下限、上限、予測スコアの4 columnsから成ります。その36マス空き局面のgame-theoretic valueが、その範囲にあるだろうという予想を意味しています。全ての"予想"が肯定的に解決されれば、元の50マス空き局面が解決できるわけです。最後の予測スコアは"予想"そのものとは関係ありませんが、難しそうなやつから順に解いていきたい場合などに参照するためのものです。

それとは別に、p006は、result\_e50[-OX]{64}\\.csv のような名前のファイルも出力します。証明が完了している場合、このファイルには証明結果としてのgame-theoretic valueの上界と下界が記録されています。

all\_p006.pyを実行すると、2587局面全てに対してp006が並列実行されます。
-->

### 2

p007 assumes that proofs using p006 are complete and that the supporting knowledge files exist. It then performs an ideal alphabeta search, outputs the number of positions explored for all visited nodes and the minimum terminal nodes required for proof. The output files are named like result\_[-OX]{64}\_abtree\\.csv . At the same time, smaller files named like result\_[-OX]{64}\_e50\\.csv are also output, containing the lower and upper game-theoretic value bounds for the 50-empty square position.

Running all_p007.py will execute p007 in parallel for all 2587 positions. Approximately 4GB of main memory might be needed per logical core.

<!--
p007は、p006を用いた証明が完了していてその根拠となるknowledgeファイルがあることを前提として、改めて理想的なalphabeta探索を行い、訪れたノード全ておよび証明のために必要な最小限の末端ノードに関する探索局面数をファイル出力します。そのファイルはresult\_[-OX]{64}\_abtree\\.csv のようなファイル名になっています。それと同時にresult\_[-OX]{64}\_e50\\.csv のような小さいファイルも出力されます。これは上記のresult\_e50[-OX]{64}\\.csvと同様でして、50マス空き局面のgame-theoretic valueの下界と上界が書かれています。

all\_p007.pyを実行すると、2587局面全てに対してp007が並列実行されます。1論理コアあたり4GB程度のメインメモリが必要かもしれません。
-->

### 3

solve33 operates under the assumption that output files from p007 for all 2587 positions are available. It proves, through depth-first search, that starting from any of position with 36 empty squares, one cannot reach a position where the player to move has more than 33 legal moves. This is to address potential bugs in Othello software implementations that can only store up to 32 moves.

<!--
solve33は、2587個の全局面に関するp007の出力ファイルが揃っている前提で動作します。36マス空きの末端局面全てに関して、それらのいかなる局面からスタートしても「手番側の合法手が33通り以上ある局面」にたどり着けないことを、実際に深さ優先探索して証明します。これは、オセロソフトの指し手保存配列が32個しか保存できない実装になっていてバグっている可能性を潰すためのものです。
-->

### 4

p008\_manyeval also assumes that output files from p007 for all 2587 positions are available. For all position with 36 empty squares, it evaluates the position using p006's static evaluation function and pairs the evaluation with the number of positions explored by Edax. The output is a massive CSV file for creating figures in the paper.

<!--
p008\_manyevalも、2587個の全局面に関するp007の出力ファイルが揃っている前提で動作します。36マス空きの末端局面全てに関して、その局面をp006の静的評価関数で評価したときの評価値を求めて、Edaxが出力した探索局面数とペアにした巨大なcsvファイル1個を出力します。論文のFigureを作るための情報を得るのに使います。
-->

### 5

check\_contradiction\_tasklist\_e50result.py is explained. Initially, when predicting the values of the 2587 positions with 50 empty squares, there were calculated bounds within which the game-theoretic value of each positions needed to be proven to demonstrate that the initial position is a draw. This script reads all the output files from p006 named result\_e50[-OX]{64}\\.csv and checks for contradictions between these initial predictions and the actual proven upper and lower bounds.

<!--
check\_contradiction\_tasklist\_e50result.pyについて説明します。そもそも最初に2587個の50マス空き局面に注目してそれらの値を予想したときに、「初期局面が引き分けであることを証明するためにはこれらの50マス空き局面の各々のgame-theoretic valueがある範囲内にあることが証明される必要がある」という下界と上界を求めました。check\_contradiction\_tasklist\_e50result.pyは、 p006の出力ファイルである result\_e50[-OX]{64}\\.csv をすべて読み込んで、その最初の予想と実際に証明された上界・下界の範囲との間に矛盾があるか無いかを調べるスクリプトです。
-->

### 6

The script make\_50\_book.py is executed after the proofs for all 2587 positions with 50 empty squares have been completed. It calculates the upper and lower bounds of the game-theoretic value for all positions that can be reached from the initial position down to 50 empty squares, and writes the results to a file. The script reads all files named result\_e50[-OX]{64}\\.csv , conducts a depth-first search to determine the values, and outputs the results to a file named result\_e50\_opening\_book.csv .

<!--
make\_50\_book.pyは、2587個の50マス空き局面全ての証明が完了したあとで実行するスクリプトです。これは初期局面から50マス空きまでの間に到達しうる全ての局面に関する、game-theoretic valueの上界と下界を求めてファイル出力します。result\_e50[-OX]{64}\\.csv　をすべて読み込んで、深さ優先探索を行いそれを求めます。出力ファイルは result\_e50\_opening\_book.csv です。
-->

### 7

convert-abtree-all.py operates under the assumption that all 2588 files, including the output files named result\_[-OX]{64}\_abtree\\.csv for all 2587 positions and result\_e50\_opening\_book.csv, are present. The script reads every line from these files, and if the proven game result is either a "win", "win or draw", "draw", "draw or lose", or "lose" for the player to move, it compresses the position and its result into 17 ASCII characters. The script then uses UNIX's sort and uniq commands to remove duplicates and writes the results to a massive text file named all\_result\_abtree\_encoded\_sorted\_unique.csv . Scripts that play moves ensuring no loss by table lookup will perform binary searches on this file.

<!--
convert-abtree-all.pyは、2587個の全局面に関するp007の出力ファイルであるresult\_[-OX]{64}\_abtree\\.csvたちと、result\_e50\_opening\_book.csvの合計2588ファイルが揃っている前提で動作します。それらのファイルのすべての行を読み、証明されている勝敗が手番側の「勝ち」「勝ちか引き分け」「引き分け」「引き分けか負け」「負け」のどれかである場合、局面とその5通りのどれなのかの情報をASCIIコード17文字に圧縮してから、UNIXのsortコマンドとuniqコマンドを使うなどして重複除去を行い、その結果をall\_result\_abtree\_encoded\_sorted\_unique.csvというファイル名の巨大なテキストファイルとして書き出します。表引きにより負けない手を指すスクリプトは、そのファイルの上で二分探索して表引きを行う実装になります。
-->

### 8

reversi\_player.py is a script that uses the all\_result\_abtree\_encoded\_sorted\_unique.csv file and the Edax\_mod2 executable to play Reversi as a player that never loses.

<!--
reversi\_player.pyは、実際にall\_result\_abtree\_encoded\_sorted\_unique.csvとEdax\_mod2の実行ファイルとを使って、絶対に負けないプレイヤーとしてリバーシをプレイするスクリプトです。
-->

#### 8.1 Steps to prepare for running reversi\_player.py

1. Download the analysis result files from https://doi.org/10.6084/m9.figshare.24420619 and place the 2587 csv files with names like knowledge\_[-OX]{64}\\.csv in the same directory as this repository's files.
1. Ensure you have enough storage space, as executing all\_p007.py and convert-abtree-all.py in the next steps will produce text files totaling about 300GB.
1. Execute the following commands:

    ```
    $ make
    $ bunzip2 -k opening_book_freq.csv.bz2
    $ sh prep-edax-and-eval.sh
    $ python3 all_p007.py
    $ python3 make_50_book.py
    $ python3 convert-abtree-all.py
    ```
1. Once you have all\_result\_abtree\_encoded\_sorted\_unique.csv and Edax\_mod2, you're all set.

## Reproducing the solution

By executing
```
$ sh prep-edax-and-eval.sh
```
you can create the Edax\_mod2 executable, which can be used to solve positions with 36 empty squares. Assuming p006 has been created using make, you can execute solve\_all\_e50\_subproblems.py to solve all 2587 positions with 50 empty squares. solve\_all\_e50\_subproblems.py is just an example of the solution process. If you truly want to solve these positions, it might be better to prepare a workflow according to the specifications of a computing cluster or similar infrastructure.
