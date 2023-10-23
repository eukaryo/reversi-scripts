
#include "Header.hpp"


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> answer;
	std::string item;
	for (char ch : s) {
		if (ch == delim) {
			if (!item.empty())
				answer.push_back(item);
			item.clear();
		}
		else {
			item += ch;
		}
	}
	if (!item.empty()) answer.push_back(item);
	return answer;
}

std::vector<std::string> get_tasklist() {

	const std::string filename = "empty50_tasklist_edax_knowledge.csv";
	std::ifstream fin;
	fin.open(filename);

	if (!fin) {
		std::cout << "fatal: we could not read " << filename << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::string line;
	std::vector<std::string> answer;
	for (int i = 0; std::getline(fin, line); ++i) {

		//obf,count,score,alpha,beta
		//------------O------OOX-----XOX----XXOO----XO-O------------------ X;,6397,0,-64,64

		const std::string obf64e50 = line.substr(0, 64);
		if (std::regex_match(line.substr(0, 67), std::regex(R"(^[-OX]{64}\sX;$)"))) {
			assert(std::count(obf64e50.begin(), obf64e50.end(), '-') == 50);
			answer.push_back(obf64e50);
		}
	}
	fin.close();
	assert(answer.size() == 2587);
	return answer;
}


std::vector<std::array<uint64_t, 2>>TASKLIST_36;
std::set<std::array<uint64_t, 2>>TASKLIST_36_SET;

void load_tasklist_e36() {

	const auto tasklist = get_tasklist();
	for (size_t i = 0; i < tasklist.size(); ++i) {
		const std::string e50 = tasklist[i];
		const std::string filename = "result_" + e50 + "_abtree.csv";
		std::ifstream fin;
		fin.open(filename);

		if (!fin) {
			std::cout << "fatal: we could not read " << filename << std::endl;
			std::exit(EXIT_FAILURE);
		}

		std::cout << "start: loading (" << i << " / " << tasklist.size() << "): " << filename << std::endl;

		std::string line;
		for (int i = 0; std::getline(fin, line); ++i) {
			if (line.size() < 64)continue;
			const std::string obf67e36 = line.substr(0, 67);
			if (std::count(obf67e36.begin(), obf67e36.end(), '-') != 36)continue;

			const std::array<uint64_t, 2> BB = board_unique(obf67e36);

			if (TASKLIST_36_SET.find(BB) == TASKLIST_36_SET.end()) {
				TASKLIST_36_SET.insert(BB);
				TASKLIST_36.push_back(BB);
			}
		}

		fin.close();
	}
	std::cout << "finish: loading files" << std::endl;

}

int32_t search_for_33(const uint64_t player, const uint64_t opponent) {

	const int32_t n_empties = _mm_popcnt_u64(~(player | opponent));
	uint64_t bb_moves = get_moves(player, opponent);

	if (n_empties <= 33) {
		if (bb_moves == 0) {
			return _mm_popcnt_u64(get_moves(opponent, player));
		}
		return _mm_popcnt_u64(bb_moves);
	}

	if (bb_moves == 0) {
		if (get_moves(opponent, player) != 0) { // pass
			return search_for_33(opponent, player);
		}
		else { // game over
			return 0;
		}
	}

	int32_t bestscore = _mm_popcnt_u64(bb_moves);

	for (uint32_t index = 0; bitscan_forward64(bb_moves, &index); bb_moves &= bb_moves - 1) {
		const uint64_t flipped = flip(index, player, opponent);
		if (flipped == opponent)continue;
		const uint64_t next_player = opponent ^ flipped;
		const uint64_t next_opponent = player ^ (flipped | (1ULL << index));

		const int32_t s = search_for_33(next_player, next_opponent);

		if (bestscore < s)bestscore = s;
		if (bestscore == 33)return 33;
	}
	return bestscore;
}

void solve() {

	load_tasklist_e36();

	std::cout << "i TASKLIST_36.size() = " << TASKLIST_36.size() << std::endl;

	std::vector<std::array<uint64_t, 2>> concerns;

	const int64_t CHANKSIZE = 1024*4;
	const int64_t outer_size = (TASKLIST_36.size() + CHANKSIZE - 1) / CHANKSIZE;
	int64_t count = 0;

#pragma omp parallel for schedule(dynamic)
	for (int64_t i = 0; i < outer_size; ++i) {
#pragma omp critical
		{
			std::cout << (++count) << " / " << outer_size << std::endl;
		}
		for (int64_t j = 0; j < CHANKSIZE; ++j) {
			const int64_t n = i * CHANKSIZE + j;
			if (n >= TASKLIST_36.size())break;
			const int32_t maxmovenum = search_for_33(TASKLIST_36[n][0], TASKLIST_36[n][1]);
			if (maxmovenum > 32) {
#pragma omp critical
				{
					std::cout << "i concern:" << bitboard2obf(TASKLIST_36[n][0], TASKLIST_36[n][1]) << std::endl;
					concerns.push_back(TASKLIST_36[n]);
				}
			}
		}
	}

	std::cout << "i RESULT: " << TASKLIST_36.size() << std::endl;
	for (const auto s : concerns) {
		std::cout << "r concern:" << bitboard2obf(s[0], s[1]) << std::endl;
	}
	if (concerns.size() == 0) {
		std::cout << "i No concern found. No position which has 33 or more possible moves can be reached from any positions with 36 empty positions which are solved." << std::endl;
	}
}

int main(int argc, char *argv[]) {

	solve();

	return 0;
}