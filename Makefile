p006: Source.cpp eval.cpp misc.cpp
	g++ -I ./ -std=c++1z -fopenmp -static-libstdc++ -O2 -flto -march=native Source.cpp eval.cpp misc.cpp -o p006
	g++ -I ./ -std=c++1z -fopenmp -static-libstdc++ -O2 -flto -march=native Source7.cpp eval.cpp misc.cpp -o p007
	g++ -I ./ -std=c++1z -fopenmp -static-libstdc++ -O2 -flto -march=native Source_manyeval.cpp eval.cpp misc.cpp -o p008_manyeval
	g++ -I ./ -std=c++1z -fopenmp -static-libstdc++ -O2 -flto -march=native solve_33.cpp eval.cpp misc.cpp -o solve33
clean:
	rm -f *.o p006 p007 p008_manyeval solve33