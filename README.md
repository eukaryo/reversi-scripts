# Othello-scripts

解析結果のファイルは https://doi.org/10.6084/m9.figshare.24420619 からダウンロードできます。ファイル内には knowledge\_[-OX]{64}\\.csv といったファイル名のcsvファイルが2587個あります。それらを本リポジトリのファイルと同じディレクトリに入れて下さい。

## knowledge\_[-OX]{64}\\.csv について

ファイル名が正規表現で knowledge\_[-OX]{64}\\.csv となっている2587個のファイルが計算結果たちです。
各rowがEdaxに読ませた結果です。columnsは左から、読ませた36マス空き局面のOBF表現、読ませた深さ、読ませた強さ(100だと完全読み)、スコアの上界、下界、Edaxが出力した探索局面数　です。

スコアの上界、下界に関しては、深さ36で強さ100のときには真の理論値に対する上界と下界です。強さ100でない場合は同じ値が入っていて、予測値を意味します。
Edaxが出力した探索局面数に関しては、正の値のときはEdaxに与えたコマンドラインオプションが
```
$ ./Edax_mod2 -solve hoge.obf -level 60 -n 1 -hash-table-size 23 -verbose 2 -alpha -3 -beta 3 -width 200
```
といった感じです。このオプションで投げ直せば全く同じ探索局面数を返すはずです。（再現性を重視しました）
負の値はこれ以外のコマンドラインオプションを使って計算したことを意味します。並列化したり、alphaとbetaの値が異なっていたりします。

## ポストプロセッシングの再現について

以降、python 3.8.10で動作確認しました。

### 0

まず、
```
$ make
$ bunzip2 -k opening_book_freq.csv.bz2
$ sh prep-edax-and-eval.sh
```
をして下さい。makeによって4つの実行ファイルができます。p006, p007, solve33, p008\_manyevalの4つです。

### 1

p006は、50マス空き局面pとknowledgeファイルkなど（詳しくはSource.cppを読んで下さい）を引数に取り、kの知識でpを弱解決できるかどうか調べて、できないならば弱解決に十分であるような"予想"の集合をファイル出力します。（弱解決できる場合もファイル出力自体はしますが、見出し行だけのcsvファイルです。"予想"の集合が空集合だということです）ここで"予想"と言っているのは、具体的にはcsvファイルのrowでして、36マス空き局面のOBF形式、スコアの下限、上限の3 columnsから成ります。その36マス空き局面のgame-theoretic valueが、その範囲にあるだろうという予想を意味しています。全ての"予想"が肯定的に解決されれば、元の50マス空き局面が解決できるわけです。

それとは別に、p006は、result\_e50[-OX]{64}\\.csv のような名前のファイルも出力します。証明が完了している場合、このファイルには証明結果としてのgame-theoretic valueの上界と下界が記録されています。

all\_p006.pyを実行すると、2587局面全てに対してp006が並列実行されます。

### 2

p007は、p006を用いた証明が完了していてその根拠となるknowledgeファイルがあることを前提として、改めて理想的なalphabeta探索を行い、訪れたノード全ておよび証明のために必要な最小限の末端ノードに関する探索局面数をファイル出力します。そのファイルはresult\_[-OX]{64}\_abtree\\.csv のようなファイル名になっています。それと同時にresult\_[-OX]{64}\_e50\\.csv のような小さいファイルも出力されます。これは上記のresult\_e50[-OX]{64}\\.csvと同様でして、50マス空き局面のgame-theoretic valueの下界と上界が書かれています。

all\_p007.pyを実行すると、2587局面全てに対してp007が並列実行されます。1論理コアあたり4GB程度のメインメモリが必要かもしれません。

### 3

solve33は、2587個の全局面に関するp007の出力ファイルが揃っている前提で動作します。36マス空きの末端局面全てに関して、それらのいかなる局面からスタートしても「手番側の合法手が33通り以上ある局面」にたどり着けないことを、実際に深さ優先探索して証明します。これは、オセロソフトの指し手保存配列が32個しか保存できない実装になっていてバグっている可能性を潰すためのものです。

### 4

p008\_manyevalも、2587個の全局面に関するp007の出力ファイルが揃っている前提で動作します。36マス空きの末端局面全てに関して、その局面をp006の静的評価関数で評価したときの評価値を求めて、Edaxが出力した探索局面数とペアにした巨大なcsvファイル1個を出力します。論文のFigureを作るための情報を得るのに使います。

### 5

check\_contradiction\_tasklist\_e50result.pyについて説明します。そもそも最初に2587個の50マス空き局面に注目してそれらの値を予想したときに、「初期局面が引き分けであることを証明するためにはこれらの50マス空き局面の各々のgame-theoretic valueがある範囲内にあることが証明される必要がある」という下界と上界を求めました。check\_contradiction\_tasklist\_e50result.pyは、result\_e50[-OX]{64}\\.csv　をすべて読み込んで、その最初の予想と実際に証明された上界・下界の範囲との間に矛盾があるか無いかを調べるスクリプトです。

### 6

make\_50\_book.pyは、2587個の50マス空き局面全ての証明が完了したあとで実行するスクリプトです。これは初期局面から50マス空きまでの間に到達しうる全ての局面に関する、game-theoretic valueの上界と下界を求めてファイル出力します。result\_e50[-OX]{64}\\.csv　をすべて読み込んで、深さ優先探索を行いそれを求めます。出力ファイルは result\_e50\_opening\_book.csv です。

### 7

convert-abtree-all.pyは、2587個の全局面に関するp007の出力ファイルであるresult\_[-OX]{64}\_abtree\\.csvたちと、result\_e50\_opening\_book.csvの合計2588ファイルが揃っている前提で動作します。それらのファイルのすべての行を読み、証明されている勝敗が手番側の「勝ち」「勝ちか引き分け」「引き分け」「引き分けか負け」「負け」のどれかである場合、局面とその5通りのどれなのかの情報をASCIIコード17文字に圧縮してから、UNIXのsortコマンドとuniqコマンドを使うなどして重複除去を行い、その結果をall\_result\_abtree\_encoded\_sorted\_unique.csvというファイル名の巨大なテキストファイルとして書き出します。表引きにより負けない手を指すスクリプトは、そのファイルの上で二分探索して表引きを行う実装になります。

### 8

reversi\_player.pyは、実際にall\_result\_abtree\_encoded\_sorted\_unique.csvとEdax\_mod2の実行ファイルとを使って、絶対に負けないプレイヤーとしてリバーシをプレイするスクリプトです。

## 求解の再現について

```
$ sh prep-edax-and-eval.sh
```
を実行すると、Edax\_mod2という実行ファイルができます。これを使って36マス空き局面を解くことができます。
makeでp006ができている前提で、solve\_all\_e50\_subproblems.pyを実行すると2587個全ての50マス空き局面について解くことができます。solve\_all\_e50\_subproblems.pyは解く流れの一例を示すために用意しただけです。本当に求解をしたい場合は、クラスタ計算機などの仕様に合わせてワークフローを用意するほうがよいでしょう。
