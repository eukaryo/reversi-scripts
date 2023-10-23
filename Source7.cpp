
#include "Header.hpp"

//#include "emilib_hashmap.hpp"


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

int32_t ComputeFinalScore(const uint64_t player, const uint64_t opponent) {

	//引数の盤面が即詰みだと仮定し、最終スコアを返す。

	const int32_t n_discs_p = _mm_popcnt_u64(player);
	const int32_t n_discs_o = _mm_popcnt_u64(opponent);

	int32_t score = n_discs_p - n_discs_o;

	//空白マスが残っている状態で両者とも打つ場所が無い場合は試合終了だが、
	//そのとき引き分けでないならば、空白マスは勝者のポイントに加算されるというルールがある。
	if (score < 0) score -= 64 - n_discs_p - n_discs_o;
	else if (score > 0) score += 64 - n_discs_p - n_discs_o;

	return score;
}

enum {
	UPPER_MODE = 0,
	LOWER_MODE = 1
};

enum {
	FIRSTLEVEL = 2,
	SECONDLEVEL = 4,
	THIRDLEVEL = 8,
};

const int START_EMPTY_NUM = 50;
const int END_EMPTY_NUM = 36;

struct Bound {
	int32_t lowerbound;
	int32_t upperbound;
};

namespace std {
template <>
struct hash<std::array<uint64_t, 2>> {
	size_t operator()(const std::array<uint64_t, 2> &x) const {
		return get_hash_code(x[0], x[1]);
	}
};
}


struct HashEntry {
	std::array<uint64_t, 2>board;
	Bound bound;
};

struct HashColumn {
	std::array<HashEntry, 3>c;
};

class MyHashTable {

private:

	std::vector<HashColumn>data;
	uint64_t log2len, bitmask;

	void init_data() {
		for (size_t i = 0; i < data.size(); ++i) {
			for (int j = 0; j < 3; ++j) {
				data[i].c[j].board[0] = 0;
				data[i].c[j].board[1] = 0;
				data[i].c[j].bound.lowerbound = -64;
				data[i].c[j].bound.upperbound = 64;
			}
		}
	}

public:

	MyHashTable(const int n) {
		log2len = n;
		bitmask = (1ULL << n) - 1ULL;
		data.resize(1ULL << n);
		init_data();
	}
	MyHashTable() : MyHashTable(10) {}

	bool find(const std::array<uint64_t, 2>&board, Bound &dest) {
		assert(board == board_unique(board));
		const uint64_t hash_value = get_hash_code(board[0], board[1]);
		const uint64_t hash_index = hash_value & bitmask;

		if (data[hash_index].c[0].board == board) {
			dest = data[hash_index].c[0].bound;
			return true;
		}

		if (data[hash_index].c[0].board == std::array<uint64_t, 2>{0, 0})return false;

		if (data[hash_index].c[1].board == board) {
			dest = data[hash_index].c[1].bound;
			return true;
		}

		if (data[hash_index].c[1].board == std::array<uint64_t, 2>{0, 0})return false;

		if (data[hash_index].c[2].board == board) {
			dest = data[hash_index].c[2].bound;
			std::swap(data[hash_index].c[1], data[hash_index].c[2]);
			return true;
		}

		return false;
	}

	void insert(const std::array<uint64_t, 2>&board, const Bound &bound) {
		assert(board == board_unique(board));
		const uint64_t hash_value = get_hash_code(board[0], board[1]);
		const uint64_t hash_index = hash_value & bitmask;

		const auto func_insert = [&](const int n) {
			data[hash_index].c[n].board = board;
			data[hash_index].c[n].bound = bound;
		};

		if (data[hash_index].c[0].board == std::array<uint64_t, 2>{0, 0} || data[hash_index].c[0].board == board) {
			func_insert(0);
			return;
		}

		const int64_t self_n_occupied = _mm_popcnt_u64(board[0] | board[1]);
		const int64_t hash_n_occupied = _mm_popcnt_u64(data[hash_index].c[0].board[0] | data[hash_index].c[0].board[1]);

		if (self_n_occupied < hash_n_occupied) {
			data[hash_index].c[2] = data[hash_index].c[1];
			data[hash_index].c[1] = data[hash_index].c[0];
			func_insert(0);
			return;
		}
		else if (self_n_occupied == hash_n_occupied) {
			if (data[hash_index].c[1].board == std::array<uint64_t, 2>{0, 0} || data[hash_index].c[1].board == board) {
				data[hash_index].c[1] = data[hash_index].c[0];
				func_insert(0);
				return;
			}
			else {
				data[hash_index].c[2] = data[hash_index].c[1];
				data[hash_index].c[1] = data[hash_index].c[0];
				func_insert(0);
				return;
			}
		}
		else {
			if (data[hash_index].c[1].board == std::array<uint64_t, 2>{0, 0} || data[hash_index].c[1].board == board) {
				func_insert(1);
				return;
			}
			else {
				data[hash_index].c[2] = data[hash_index].c[1];
				func_insert(1);
				return;
			}
		}
	}
};



typedef std::unordered_map<std::array<uint64_t, 2>, Bound> StaticHashMap;

typedef std::unordered_map<std::array<uint64_t, 2>, Bound> HashMap;

//typedef emilib::HashMap<std::array<uint64_t, 2>, Bound> HashTable;

std::array<MyHashTable, 2>thirdlevel_table;

MyHashTable secondlevel_table;

HashMap firstlevel_table;

StaticHashMap exact_knowledge;
std::unordered_map<std::array<uint64_t, 2>, int64_t> exact_knowledge_read_count;

std::unordered_map<std::array<uint64_t, 2>, int32_t> kifu_freq_data;

std::unordered_map<std::array<uint64_t, 2>, int32_t> override_evaluation_table;

const int32_t output_max_abs = 64;

std::vector<std::array<uint64_t, 2>>TO_SEARCH_ORDER;

std::unordered_map<std::array<uint64_t, 2>, std::array<int32_t, 2>>TO_SEARCH;

constexpr int BUFSIZE = 2 * 1024 * 1024;

void output_TO_SEARCH_csv(const std::string &filename) {

	std::cout << "start: output_TO_SEARCH_csv" << std::endl;
	const auto start = std::chrono::system_clock::now(); // 計測開始時間

	static char buf[BUFSIZE];//大きいのでスタック領域に置きたくないからstatic。（べつにmallocでもstd::vectorでもいいんだけど）

	std::ofstream writing_file;
	writing_file.rdbuf()->pubsetbuf(buf, BUFSIZE);
	writing_file.open(filename, std::ios::out);

	writing_file << "obf,lower_bound,upper_bound" << std::endl;

	assert(TO_SEARCH_ORDER.size() == TO_SEARCH.size());

	for (size_t i = 0; i < TO_SEARCH_ORDER.size(); ++i) {
		const std::array<uint64_t, 2>BB = board_unique(TO_SEARCH_ORDER[i][0], TO_SEARCH_ORDER[i][1]);
		assert(BB == TO_SEARCH_ORDER[i]);
		const std::string obf = bitboard2obf(BB[0], BB[1]);

		if(!(-64 <= TO_SEARCH[BB][0] && TO_SEARCH[BB][0] <= TO_SEARCH[BB][1] && TO_SEARCH[BB][1] <= 64)){
			std::cout << "error: output_TO_SEARCH_csv: " << obf << "," << TO_SEARCH[BB][0] << "," << TO_SEARCH[BB][1] << std::endl;
		}
		assert(-64 <= TO_SEARCH[BB][0] && TO_SEARCH[BB][0] <= TO_SEARCH[BB][1] && TO_SEARCH[BB][1] <= 64);

		const int32_t n_empties = _mm_popcnt_u64(~(BB[0] | BB[1]));
		if (n_empties == END_EMPTY_NUM){
			assert(exact_knowledge_read_count.find(BB) != exact_knowledge_read_count.end());
			writing_file << obf << "," << TO_SEARCH[BB][0] << "," << TO_SEARCH[BB][1] << "," << exact_knowledge_read_count[BB] << std::endl;
		}
		else{
			writing_file << obf << "," << TO_SEARCH[BB][0] << "," << TO_SEARCH[BB][1] << "," << std::endl;
		}
	}

	writing_file.close();

	const auto end = std::chrono::system_clock::now();  // 計測終了時間
	const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); //処理に要した時間をミリ秒に変換
	std::cout << "finish: output_TO_SEARCH_csv: elapsed time = " << elapsed << " ms" << std::endl;
}

void output_RESULT_csv(const std::string &obf, const int32_t alpha, const int32_t beta, const int32_t score) {

	std::cout << "start: output_RESULT_csv" << std::endl;

	static char buf[BUFSIZE];//大きいのでスタック領域に置きたくないからstatic。（べつにmallocでもstd::vectorでもいいんだけど）

	std::ofstream writing_file;
	writing_file.rdbuf()->pubsetbuf(buf, BUFSIZE);
	writing_file.open(std::string("result_")+obf.substr(0, 64)+"_e50.csv", std::ios::out);

	int32_t score_lowerbound = 100, score_upperbound = 100;
	if (alpha < score && score < beta) {
		score_lowerbound = score;
		score_upperbound = score;
	}
	else if (score <= alpha) {
		score_lowerbound = -64;
		score_upperbound = score;
	}
	else if (beta <= score) {
		score_lowerbound = score;
		score_upperbound = 64;
	}

	assert(-64 <= score_lowerbound && score_lowerbound <= score_upperbound && score_upperbound <= 64);

	writing_file << "obf,score_lowerbound,score_upperbound" << std::endl;
	writing_file << obf << "," << score_lowerbound << "," << score_upperbound << std::endl;

	writing_file.close();

	std::cout << "finish: output_RESULT_csv" << std::endl;
}

//firstlevelの仕事は、END_EMPTY_NUMまで"最適なルート"で読んで、全ての未決定な末端ノードを出力すること。
//ここでいう"最適なルート"とは、
//(1)ある合法手で進んだ先をthirdlevelで読んで、exact_scoreが確定値であるか、fail-high/lowすることが確実ならば、読まずに引き返すこと。
//(2)さもなくば、各合法手で進んだ先をsecondlevelで読んで、スコアが高い順に読むこと。
//とする。

//secondlevelの仕事は、END_EMPTY_NUMまで読んで、"真の値"を返すこと。
//ここでいう"真の値"とは、
//(1)exact_knowledgeが無いなら静的評価関数の値のこと
//(2)exact_knowledgeが正確な値であるならその値のこと
//(3)exact_knowledgeに幅があり静的評価関数の値がその範囲内ならば、静的評価関数の値のこと
//(4)exact_knowledgeに幅があり静的評価関数の値がその範囲外ならば、exact_knowledgeの範囲のうち静的評価関数の値に最も近い値のこと
//とする。

//上記の「静的評価関数の値」は、局面を引数に取り整数を返すedaxの評価関数を基本とするが、override_evaluation_tableに載っている局面についてはそれの値を優先する。

//thirdlevelの仕事は、END_EMPTY_NUMまで読んで、"真の値の区間"の上端か下端を返すこと。
//ここでいう"真の値の区間"とは、
//(1)exact_knowledgeが無いなら[-64,64]のこと
//(2)exact_knowledgeがあるならその区間のこと
//とする。
//modeは0か1で、0が上端、1が下端とする。


int32_t static_evaluation_search(const uint64_t player, const uint64_t opponent, int32_t alpha, const int32_t beta, const int32_t depth) {

	if (depth <= 0) {
		int32_t score = EvaluatePosition0(player, opponent);
		score = (score / 2) * 2;
		return score;
	}

	const int32_t old_alpha = alpha;
	const int32_t n_empties = _mm_popcnt_u64(~(player | opponent));
	uint64_t bb_moves = get_moves(player, opponent);

	if (bb_moves == 0) {
		if (get_moves(opponent, player) != 0) { // pass
			return -static_evaluation_search(opponent, player, -beta, -alpha, depth);
		}
		else { // game over
			return ComputeFinalScore(player, opponent);
		}
	}

	int32_t bestscore = std::numeric_limits<int32_t>::min();

	if (depth == 1) {
		for (uint32_t index = 0; bitscan_forward64(bb_moves, &index); bb_moves &= bb_moves - 1) {
			uint64_t flipped = flip(index, player, opponent);
			if (flipped == opponent)return 64; // wipeout
			const uint64_t next_player = opponent ^ flipped;
			const uint64_t next_opponent = player ^ (flipped | (1ULL << index));

			int32_t s = -EvaluatePosition0(next_player, next_opponent);
			s = (s / 2) * 2;

			if (bestscore < s)bestscore = s;
			if (beta <= s)return bestscore;
		}
		return bestscore;
	}

	uint64_t flipped[MAX_MOVE] = {};
	int64_t moves[MAX_MOVE] = {}, move_values[MAX_MOVE] = {}, movenum = 0;

	for (uint32_t index = 0; bitscan_forward64(bb_moves, &index); bb_moves &= bb_moves - 1) {
		moves[movenum] = index;
		flipped[movenum] = flip(index, player, opponent);
		if (flipped[movenum] == opponent)return 64; // wipeout
		const uint64_t next_player = opponent ^ flipped[movenum];
		const uint64_t next_opponent = player ^ (flipped[movenum] | (1ULL << index));

		move_values[movenum++] = integrated_move_scoring(next_player, next_opponent, index);
	}

	//sort moves by move_values in descending order.
	for (int i = 0; i < movenum - 1; ++i) {
		int32_t max_value_index = i;
		for (int j = i + 1; j < movenum; ++j)if (move_values[max_value_index] < move_values[j])max_value_index = j;
		if (i != max_value_index) {
			std::swap(flipped[i], flipped[max_value_index]);
			std::swap(moves[i], moves[max_value_index]);
			std::swap(move_values[i], move_values[max_value_index]);
		}
	}
	for (int i = 0; i < movenum - 1; ++i) {
		assert(move_values[i] >= move_values[i + 1]);
	}

	int32_t move_index = 0;

	//first move in PV node
	if (alpha + 1 < beta) {
		const uint64_t next_player = opponent ^ flipped[move_index];
		const uint64_t next_opponent = player ^ (flipped[move_index] | (1ULL << moves[move_index]));

		const int32_t s = -static_evaluation_search(next_player, next_opponent, -beta, -alpha, depth - 1);
		if (bestscore < s)bestscore = s;
		if (beta <= s)return bestscore;
		if (alpha < s)alpha = s;

		move_index = 1;
	}

	//all moves in Null-Window node, or second and subsequent moves in PV node
	for (; move_index < movenum; ++move_index) {
		const uint64_t next_player = opponent ^ flipped[move_index];
		const uint64_t next_opponent = player ^ (flipped[move_index] | (1ULL << moves[move_index]));

		const int32_t s1 = -static_evaluation_search(next_player, next_opponent, -(alpha + 1), -alpha, depth - 1);
		if (bestscore < s1)bestscore = s1;
		if (beta <= bestscore)return bestscore;
		if (alpha < s1) {
			alpha = s1;
			const int32_t s2 = -static_evaluation_search(next_player, next_opponent, -beta, -alpha, depth - 1);
			if (bestscore < s2)bestscore = s2;
			if (beta <= bestscore)return bestscore;
			if (alpha < s2)alpha = s2;
		}
	}

	assert(-64 <= bestscore && bestscore <= 64);
	return bestscore;
}


constexpr int32_t STATIC_EVALUATION_FUNCTION_SEARCH_THRESHOLD = 11;

int32_t static_evaluation_function(const std::array<uint64_t, 2> &BB) {

	if (override_evaluation_table.find(BB) != override_evaluation_table.end()) {
		const int32_t x = override_evaluation_table[BB];
		return (x / 2) * 2;
	}


	int32_t score = EvaluatePosition0(BB[0], BB[1]);
	score = (score / 2) * 2;

	if (score < -STATIC_EVALUATION_FUNCTION_SEARCH_THRESHOLD || STATIC_EVALUATION_FUNCTION_SEARCH_THRESHOLD < score)return score;

	const int32_t alpha = std::min(-3, score - 1);
	const int32_t beta = std::max(3, score + 1);

	score = static_evaluation_search(BB[0], BB[1], alpha, beta, 2);
	score = (score / 2) * 2;

	return score;
}

int32_t static_evaluation_function_without_override(const std::array<uint64_t, 2> &BB) {

	int32_t score = EvaluatePosition0(BB[0], BB[1]);
	score = (score / 2) * 2;

	if (score < -STATIC_EVALUATION_FUNCTION_SEARCH_THRESHOLD || STATIC_EVALUATION_FUNCTION_SEARCH_THRESHOLD < score)return score;

	const int32_t alpha = std::min(-3, score - 1);
	const int32_t beta = std::max(3, score + 1);

	score = static_evaluation_search(BB[0], BB[1], alpha, beta, 2);
	score = (score / 2) * 2;

	return score;
}

template<int32_t SEARCH_LEVEL>int32_t PVS_midgame(const uint64_t player, const uint64_t opponent, int32_t alpha, const int32_t beta) {

	assert(SEARCH_LEVEL == FIRSTLEVEL ||
		SEARCH_LEVEL == SECONDLEVEL ||
		SEARCH_LEVEL == THIRDLEVEL + UPPER_MODE ||
		SEARCH_LEVEL == THIRDLEVEL + LOWER_MODE);

	const int32_t n_empties = _mm_popcnt_u64(~(player | opponent));
	const std::array<uint64_t, 2>BB = board_unique(player, opponent);

	Bound old_bound;
	old_bound.lowerbound = -64;
	old_bound.upperbound = 64;
	if (SEARCH_LEVEL == FIRSTLEVEL) {
		if (firstlevel_table.find(BB) != firstlevel_table.end()) {
			old_bound = firstlevel_table[BB];
		}
	}
	else if (SEARCH_LEVEL == SECONDLEVEL) {
		secondlevel_table.find(BB, old_bound);
	}
	else if (SEARCH_LEVEL & THIRDLEVEL) {
		thirdlevel_table[SEARCH_LEVEL % 2].find(BB, old_bound);
	}

	if (old_bound.lowerbound == old_bound.upperbound)return old_bound.lowerbound;
	if (old_bound.upperbound <= alpha)return old_bound.upperbound;
	if (beta <= old_bound.lowerbound)return old_bound.lowerbound;

	if (SEARCH_LEVEL == FIRSTLEVEL) {
		if (output_max_abs < 64) {
			const auto s = PVS_midgame<SECONDLEVEL>(player, opponent, -64, 64);
			if (s < -output_max_abs || output_max_abs < s) {
				return s;
			}
		}
	}

	uint64_t bb_moves = get_moves(player, opponent);
	const int32_t old_alpha = alpha;
	int32_t bestscore = std::numeric_limits<int32_t>::min();

	if (n_empties == END_EMPTY_NUM) {

		if (bb_moves == 0) {
			if (get_moves(opponent, player) != 0) { // pass
				return -PVS_midgame<SEARCH_LEVEL ^ (SEARCH_LEVEL / 8)>(opponent, player, -beta, -alpha);
			}
			else { // game over
				return ComputeFinalScore(player, opponent);
			}
		}
		else {
			//1手詰め局面なら+64を返す
			for (uint32_t index = 0; bitscan_forward64(bb_moves, &index); bb_moves &= bb_moves - 1) {
				if (flip(index, player, opponent) == opponent)return 64;
			}
		}

		if (SEARCH_LEVEL == FIRSTLEVEL) {

			assert(exact_knowledge.find(BB) != exact_knowledge.end());

			if (TO_SEARCH.find(BB) == TO_SEARCH.end()) {
				Bound k;
				k = exact_knowledge[BB];
				TO_SEARCH[BB] = { k.lowerbound,k.upperbound };
				TO_SEARCH_ORDER.push_back(BB);
			}

			const int32_t score = PVS_midgame<SECONDLEVEL>(player, opponent, -64, 64);
			return (score / 2) * 2;
		}
		else if (SEARCH_LEVEL == SECONDLEVEL) {

			if (exact_knowledge.find(BB) != exact_knowledge.end()) {
				Bound k;
				k = exact_knowledge[BB];
				assert((k.lowerbound / 2) * 2 == k.lowerbound);
				assert((k.upperbound / 2) * 2 == k.upperbound);
				if (k.lowerbound == k.upperbound)return k.lowerbound;
				if (beta <= k.lowerbound)return k.lowerbound;
				if (k.upperbound <= alpha)return k.upperbound;

				int32_t score = static_evaluation_function(BB);
				score = (score / 2) * 2;

				if (k.lowerbound <= score && score <= k.upperbound)return score;
				if (score < k.lowerbound)return k.lowerbound;
				if (k.upperbound < score)return k.upperbound;
				assert(false);
			}
			else {
				int32_t score = static_evaluation_function(BB);
				score = (score / 2) * 2;
				return score;
			}
		}
		else if (SEARCH_LEVEL & THIRDLEVEL) {
			if (exact_knowledge.find(BB) != exact_knowledge.end()) {
				Bound k;
				k = exact_knowledge[BB];
				return (SEARCH_LEVEL == THIRDLEVEL + UPPER_MODE) ? k.upperbound : k.lowerbound;
			}
			else {
				return (SEARCH_LEVEL == THIRDLEVEL + UPPER_MODE) ? 64 : -64;
			}
		}
		assert(false);
		return 0;
	}

	Bound exact_bound;
	exact_bound.lowerbound = -64;
	exact_bound.upperbound = -64;

	if (bb_moves == 0) { // special cases
		if (get_moves(opponent, player) != 0) { // pass
			bestscore = -PVS_midgame<SEARCH_LEVEL ^ (SEARCH_LEVEL / 8)>(opponent, player, -beta, -alpha);
		}
		else { // game over
			return ComputeFinalScore(player, opponent);
		}
	}
	else {

		uint64_t flipped[MAX_MOVE] = {};
		int64_t moves[MAX_MOVE] = {}, move_values[MAX_MOVE] = {}, movenum = 0;

		for (uint32_t index = 0; bitscan_forward64(bb_moves, &index); bb_moves &= bb_moves - 1) {
			moves[movenum] = index;
			flipped[movenum] = flip(index, player, opponent);
			if (flipped[movenum] == opponent)return 64; // wipeout
			const uint64_t next_player = opponent ^ flipped[movenum];
			const uint64_t next_opponent = player ^ (flipped[movenum] | (1ULL << index));

			if (SEARCH_LEVEL == FIRSTLEVEL) {
				Bound b;
				b.lowerbound = -PVS_midgame<THIRDLEVEL + UPPER_MODE>(next_player, next_opponent, -64, 64);
				b.upperbound = -PVS_midgame<THIRDLEVEL + LOWER_MODE>(next_player, next_opponent, -64, 64);
				assert(-64 <= b.lowerbound && b.lowerbound <= b.upperbound && b.upperbound <= 64);
				if (beta <= b.lowerbound){

					if (TO_SEARCH.find(BB) == TO_SEARCH.end()) {
						TO_SEARCH[BB] = { b.lowerbound,b.upperbound };
						TO_SEARCH_ORDER.push_back(BB);
					}

					PVS_midgame<FIRSTLEVEL>(next_player, next_opponent, -beta, -alpha);

					return b.lowerbound;
				}
				else if (b.upperbound <= alpha) {
					bestscore = std::max(bestscore, b.upperbound);
				}
				else if (b.lowerbound == b.upperbound && alpha < b.lowerbound && b.upperbound < beta) {
					bestscore = std::max(bestscore, b.upperbound);
				}

				exact_bound.lowerbound = std::max(exact_bound.lowerbound, b.lowerbound);
				exact_bound.upperbound = std::max(exact_bound.upperbound, b.upperbound);
				
				const int64_t score = PVS_midgame<SECONDLEVEL>(next_player, next_opponent, -64, 64);

				move_values[movenum] = -score * (1LL << 50);
				move_values[movenum] += b.lowerbound * (1LL << 56);

				const std::array<uint64_t, 2>NEXT_BB = board_unique(next_player, next_opponent);
				if (kifu_freq_data.find(NEXT_BB) != kifu_freq_data.end())move_values[movenum] += int64_t(kifu_freq_data[BB]) * (1LL << 33);

				move_values[movenum] += integrated_move_scoring(next_player, next_opponent, index);

				++movenum;
			}
			else move_values[movenum++] = integrated_move_scoring(next_player, next_opponent, index);
		}

		// if (SEARCH_LEVEL == FIRSTLEVEL) {
		// 	assert(movenum == 0);
		// }

		// if (movenum == 0) {//THIRD_LEVEL-search confirmed that all legal moves cause fail-low.
		// 	assert(SEARCH_LEVEL == FIRSTLEVEL);
		// 	assert(-64 <= bestscore && bestscore < beta);
		// 	return bestscore;
		// }

		//sort moves by move_values in descending order.
		for (int i = 0; i < movenum - 1; ++i) {
			int32_t max_value_index = i;
			for (int j = i + 1; j < movenum; ++j)if (move_values[max_value_index] < move_values[j])max_value_index = j;
			if (i != max_value_index) {
				std::swap(flipped[i], flipped[max_value_index]);
				std::swap(moves[i], moves[max_value_index]);
				std::swap(move_values[i], move_values[max_value_index]);
			}
		}
		for (int i = 0; i < movenum - 1; ++i) {
			assert(move_values[i] >= move_values[i + 1]);
		}

		int32_t move_index = 0;

		if (SEARCH_LEVEL == FIRSTLEVEL){

			if (TO_SEARCH.find(BB) == TO_SEARCH.end()) {
				assert(-64 <= exact_bound.lowerbound && exact_bound.lowerbound <= exact_bound.upperbound && exact_bound.upperbound <= 64);
				TO_SEARCH[BB] = { exact_bound.lowerbound,exact_bound.upperbound };
				TO_SEARCH_ORDER.push_back(BB);
			}
		}
		//first move in PV node
		if (alpha + 1 < beta) {
			const uint64_t next_player = opponent ^ flipped[move_index];
			const uint64_t next_opponent = player ^ (flipped[move_index] | (1ULL << moves[move_index]));

			const int32_t s = -PVS_midgame<SEARCH_LEVEL ^ (SEARCH_LEVEL / 8)>(next_player, next_opponent, -beta, -alpha);
			if (bestscore < s)bestscore = s;
			if (beta <= s)goto END_PHASE;
			if (alpha < s)alpha = s;

			move_index = 1;
		}

		//all moves in Null-Window node, or second and subsequent moves in PV node
		for (; move_index < movenum; ++move_index) {

			if (move_values[move_index] == std::numeric_limits<int64_t>::min()) {
				assert(0);//SEARCH_LEVEL == FIRSTLEVEL);
				assert(move_index);
				break;
			}

			const uint64_t next_player = opponent ^ flipped[move_index];
			const uint64_t next_opponent = player ^ (flipped[move_index] | (1ULL << moves[move_index]));

			const int32_t s1 = -PVS_midgame<SEARCH_LEVEL ^ (SEARCH_LEVEL / 8)>(next_player, next_opponent, -(alpha + 1), -alpha);
			if (bestscore < s1)bestscore = s1;
			if (beta <= bestscore) break;
			if (alpha < s1) {
				alpha = s1;
				const int32_t s2 = -PVS_midgame<SEARCH_LEVEL ^ (SEARCH_LEVEL / 8)>(next_player, next_opponent, -beta, -alpha);
				if (bestscore < s2)bestscore = s2;
				if (beta <= bestscore) break;
				if (alpha < s2)alpha = s2;
			}
		}
	}

END_PHASE:;

	if (old_alpha < bestscore && bestscore < beta) {
		Bound b;
		b.lowerbound = b.upperbound = bestscore;
		if (SEARCH_LEVEL == FIRSTLEVEL)firstlevel_table[BB] = b;
		if (SEARCH_LEVEL == SECONDLEVEL)secondlevel_table.insert(BB, b);
		if (SEARCH_LEVEL & THIRDLEVEL)thirdlevel_table[SEARCH_LEVEL % 2].insert(BB, b);
	}
	else if (bestscore <= old_alpha) {
		old_bound.upperbound = std::min(bestscore, old_bound.upperbound);
		assert(old_bound.lowerbound <= old_bound.upperbound);
		if (SEARCH_LEVEL == FIRSTLEVEL)firstlevel_table[BB] = old_bound;
		if (SEARCH_LEVEL == SECONDLEVEL)secondlevel_table.insert(BB, old_bound);
		if (SEARCH_LEVEL & THIRDLEVEL)thirdlevel_table[SEARCH_LEVEL % 2].insert(BB, old_bound);
	}
	else if (beta <= bestscore) {
		old_bound.lowerbound = std::max(bestscore, old_bound.lowerbound);
		assert(old_bound.lowerbound <= old_bound.upperbound);
		if (SEARCH_LEVEL == FIRSTLEVEL)firstlevel_table[BB] = old_bound;
		if (SEARCH_LEVEL == SECONDLEVEL)secondlevel_table.insert(BB, old_bound);
		if (SEARCH_LEVEL & THIRDLEVEL)thirdlevel_table[SEARCH_LEVEL % 2].insert(BB, old_bound);
	}
	else {
		assert(false);
	}

	assert(-64 <= bestscore && bestscore <= 64);
	return bestscore;
}

void load_kifu_freq() {

	constexpr int THRESHOLD = 10;

	kifu_freq_data.clear();

	std::ifstream fin;
	fin.open("opening_book_freq.csv");
	if (!fin) {
		std::cout << "warning: we could not read opening_book_freq.csv" << std::endl;
		return;
	}

	std::cout << "start: load_kifu_freq" << std::endl;

	std::string line;
	for (int i = 0; std::getline(fin, line); ++i) {
		std::vector<std::string> x = split(line, ',');
		if (x.size() != 3)continue;

		if (IsObfFormat(x[2]) == false)continue;

		try {
			const int32_t freq = std::stoi(x[0]);
			const int32_t n_empties = std::stoi(x[1]);
			const std::array<uint64_t, 2>BB = board_unique(x[2]);
			if (n_empties != _mm_popcnt_u64(~(BB[0] | BB[1]))) {
				std::cout << "warning: failed to validate a line: " << line << std::endl;
				continue;
			}
			if (END_EMPTY_NUM <= n_empties && n_empties < START_EMPTY_NUM && freq >= THRESHOLD) {
				assert(kifu_freq_data.find(BB) == kifu_freq_data.end());
				kifu_freq_data[BB] = freq;
			}
		}
		catch (...) {
			std::cout << "warning: failed to read a knowledge line: " << line << std::endl;
			continue;
		}

	}
	fin.close();

	std::cout << "finish: load_kifu_freq" << std::endl;


}

void load_knowledge(const std::string filename) {

	//knowledgeファイルはcsv形式で、（見出し行以外の）各行は
	//obf,depth,accuracy,score_lowerbound,score_upperbound,nodes
	//になっているとする。

	//完全読みの結果が書かれているときはdepth==(空きマス数),accuracy==100である。
	//さもなくば完全読みではない。（静的評価関数よりは信頼できると考えられるのでoverride_knowledgeに格納する。）

	//複数の行に同じ局面の情報が書いてあることがある。ただし、完全読みの場合はそれらは相互矛盾しておらず、真の値は共通部分にあると仮定して良い。
	//完全読みでない行において、score_lowerboundとscore_upperboundの値は必ず等しいと仮定して良い。

	//1行目およびそれ以外の行が見出し行（"obf,lower_exact,upper_exact"など）になっていることもある。それらは単に無視する。

	//要件: 例えば、20手73%[+3,+3]と36手99%[+4,+64]があって、静的評価関数は+18のとき、override_knowledgeには+4が登録されてほしい。
	//      (静的評価関数よりは20手読みの値のほうが信頼できるので+3を使ってほしいが、99%読みのレンジ内にclampしてほしい)
	//      これは、knowledgeファイル内で登場する順番にかかわらずそうなってほしい。

	assert(override_evaluation_table.size() == 0);

	typedef struct {
		int64_t strength;
		int32_t score;
	}V_POINT;
	typedef struct {
		int64_t strength;
		Bound b;
	}V_RANGE;
	std::map<std::array<uint64_t, 2>, V_POINT>tmp_override_data_point;
	std::map<std::array<uint64_t, 2>, V_RANGE>tmp_override_data_range;

	const auto func_intersection = [&](const Bound b1, const Bound b2) {
		Bound b;
		b.lowerbound = std::max(b1.lowerbound, b2.lowerbound);
		b.upperbound = std::min(b1.upperbound, b2.upperbound);
		assert(-64 <= b.lowerbound && b.lowerbound <= b.upperbound && b.upperbound <= 64);
		return b;
	};

	std::ifstream fin;
	fin.open(filename);
	if (!fin) {
		std::cout << "warning: we could not read " << filename << std::endl;
		return;
	}
	std::string line;
	for (int i = 0; std::getline(fin, line); ++i) {
		std::vector<std::string> x = split(line, ',');
		if (x.size() != 6)continue;//[0]から順に,obf,depth,accuracy,score_lowerbound,score_upperbound,read_count と仮定する。

		if (IsObfFormat(x[0]) == false)continue;

		try {

			Bound d;
			const auto BB = board_unique(x[0]);
			const int64_t n_empties = _mm_popcnt_u64(~(BB[0] | BB[1]));

			const int64_t depth = std::stoi(x[1]);
			const int64_t accuracy = std::stoi(x[2]);
			assert(0 <= depth && depth <= n_empties);
			assert(0 <= accuracy && accuracy <= 100);

			d.lowerbound = std::stoi(x[3]);
			d.upperbound = std::stoi(x[4]);//ちなみに、std::stoiは後ろに数値でない文字列がくっついていると単に無視する。
			const int64_t read_count = std::stoll(x[5]);
			assert(-64 <= d.lowerbound && d.lowerbound <= d.upperbound && d.upperbound <= 64);

			if (depth == n_empties && accuracy == 100) {

				assert((d.lowerbound / 2) * 2 == d.lowerbound);
				assert((d.upperbound / 2) * 2 == d.upperbound);

				if (exact_knowledge.find(BB) == exact_knowledge.end()) {
					exact_knowledge[BB] = d;
					exact_knowledge_read_count[BB] = read_count;
				}
				else {
					exact_knowledge[BB] = func_intersection(exact_knowledge[BB], d);
					if(std::abs(read_count) < std::abs(exact_knowledge_read_count[BB])){
						exact_knowledge_read_count[BB] = read_count;
					}
				}
			}
			else {
				if(d.lowerbound == d.upperbound) {
					V_POINT v;
					v.strength = depth * 1000 + accuracy;
					v.score = (d.lowerbound / 2) * 2;
					if (tmp_override_data_point.find(BB) == tmp_override_data_point.end()) {
						tmp_override_data_point[BB] = v;
					}
					else if (tmp_override_data_point[BB].strength < v.strength) {
						tmp_override_data_point[BB] = v;
					}
				}
				else {
					V_RANGE v;
					v.strength = depth * 1000 + accuracy;
					v.b = d;
					if (tmp_override_data_range.find(BB) == tmp_override_data_range.end()) {
						tmp_override_data_range[BB] = v;
					}
					else if (tmp_override_data_range[BB].strength < v.strength) {
						tmp_override_data_range[BB] = v;
					}
				}
			}
		}
		catch (...) {
			std::cout << "warning: failed to read a knowledge line: " << line << std::endl;
			continue;
		}
	}
	fin.close();

	for (const auto x : tmp_override_data_point) {
		if (tmp_override_data_range.find(x.first) == tmp_override_data_range.end()) {
			override_evaluation_table[x.first] = x.second.score;
		}
		else if (tmp_override_data_range[x.first].strength <= x.second.strength) {
			override_evaluation_table[x.first] = x.second.score;
		}
		else {
			const Bound b = tmp_override_data_range[x.first].b;
			override_evaluation_table[x.first] = std::clamp(x.second.score, b.lowerbound, b.upperbound);
		}
	}

	for (const auto x : tmp_override_data_range) {
		if (tmp_override_data_point.find(x.first) == tmp_override_data_point.end()) {
			const int32_t eval_score = (static_evaluation_function_without_override(x.first) / 2)* 2;
			override_evaluation_table[x.first] = std::clamp(eval_score, x.second.b.lowerbound, x.second.b.upperbound);
		}
		else {
			assert(override_evaluation_table.find(x.first) != override_evaluation_table.end());
		}
	}

	std::cout << "info: number of exact knowledge = " << exact_knowledge.size() << std::endl;
	std::cout << "info: number of override knowledge = " << override_evaluation_table.size() << std::endl;
}


void p007_solve_root(const uint64_t player, const uint64_t opponent, int32_t alpha, const int32_t beta) {

	const int32_t n_empties = _mm_popcnt_u64(~(player | opponent));

	assert(n_empties == START_EMPTY_NUM);

	const std::array<uint64_t, 2>BB = board_unique(player, opponent);

	const std::string obf = bitboard2obf(BB[0], BB[1]);

	std::cout << "start(unique):" << obf << std::endl;

	const int32_t score = PVS_midgame<FIRSTLEVEL>(player, opponent, alpha, beta);

	std::cout << "p007_solve_root (alpha = " << alpha << ", beta = " << beta << ") : score = " << score << std::endl;

	output_RESULT_csv(obf, alpha, beta, score);

	std::cout << "TO_SEARCH.size() = " << TO_SEARCH.size() << std::endl;
	output_TO_SEARCH_csv("result_" + obf.substr(0, 64) + "_abtree.csv");
}


std::map<std::string, std::string>parse_args(int argc, char **argv) {

	std::vector<std::string>args;
	for (int i = 1; i < argc; ++i) {
		args.push_back(argv[i]);
	}

	std::map<std::string, std::string>input;
	input["alpha"] = "-64";
	input["beta"] = "64";
	input["max"] = "64";
	input["eval-file"] = "edax_eval_weight.json";

	std::string opcode = "";

	for (uint64_t i = 0; i < args.size(); ++i) {
		if (opcode == "") {
			if (args[i] == "-o" || args[i] == "--obf") {
				opcode = "obf";
			}
			else if (args[i] == "-a" || args[i] == "--alpha") {
				opcode = "alpha";
			}
			else if (args[i] == "-b" || args[i] == "--beta") {
				opcode = "beta";
			}
			else if (args[i] == "-e" || args[i] == "--eval-file") {
				opcode = "eval-file";
			}
			else if (args[i] == "-k" || args[i] == "--knowledge-file") {
				opcode = "knowledge-file";
			}
			else if (args[i] == "-t" || args[i] == "--test") {
				input["test"] = "true";
			}
			else if (args[i] == "-m" || args[i] == "--max") {
				opcode = "max";
			}
			else {
				std::cout << "error: command line argument is invalid. error_code = 1 (cf. parse_args function in the source code)" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
		else if (opcode == "obf") {
			if (std::regex_match(args[i], std::regex(R"(^[-OX]{64}\s[XO];$)"))) {
				input[opcode] = args[i];
				const std::array<uint64_t, 2>BB = board_unique(args[i]);
				const int n_empties = _mm_popcnt_u64(~(BB[0] | BB[1]));
				if (n_empties != START_EMPTY_NUM) {
					std::cout << "error: command line argument is invalid. error_code = 2 (cf. parse_args function of the source code)" << std::endl;
					std::exit(EXIT_FAILURE);
				}
				opcode = "";
			}
			else {
				std::cout << "error: command line argument is invalid. error_code = 3 (cf. parse_args function of the source code)" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
		else if (opcode == "alpha" || opcode == "beta" || opcode == "max") {
			if (std::regex_match(args[i], std::regex(R"(-?[0-9]{1,2})"))) {
				input[opcode] = args[i];
				const int num = std::stoi(input[opcode]);
				if (num < -64 || 64 < num || (opcode == "max" && num < 0)) {
					std::cout << "error: command line argument is invalid. error_code = 4 (cf. main function of the source code)" << std::endl;
					std::exit(EXIT_FAILURE);
				}
				opcode = "";
			}
			else {
				std::cout << "error: command line argument is invalid. error_code = 5 (cf. main function of the source code)" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
		else if (opcode == "eval-file" || opcode == "knowledge-file") {
			input[opcode] = args[i];
			opcode = "";
		}
		else {
			std::cout << "error: command line argument is invalid. error_code = 6 (cf. main function of the source code)" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	if (std::stoi(input["alpha"]) >= std::stoi(input["beta"])) {
		std::cout << "error: command line argument is invalid. error_code = 7 (cf. main function of the source code)" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if (input.find("obf") == input.end()) {
		std::cout << "error: command line argument is invalid. error_code = 8 (cf. main function of the source code)" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return input;
}

int main(int argc, char **argv) {

	init_hash_rank();

	constexpr int hash_size = 23;
	secondlevel_table = MyHashTable(hash_size);
	thirdlevel_table[0] = MyHashTable(hash_size);
	thirdlevel_table[1] = MyHashTable(hash_size);

	std::map<std::string, std::string>args = parse_args(argc, argv);

	init_eval_json(args["eval-file"]);

	if (args.find("test") != args.end()) {
		unittest_misc(100000, 123456);
	}

	uint64_t player = 0, opponent = 0;
	obf2bitboard(args["obf"], player, opponent);

	if (args.find("knowledge-file") != args.end()) {
		load_knowledge(args["knowledge-file"]);
	}
	load_kifu_freq();

	//output_max_abs = 64;//std::stoi(args["max"]);

	std::cout << "start(given):" << args["obf"] << std::endl;
	const auto start = std::chrono::system_clock::now();
	p007_solve_root(player, opponent, std::stoi(args["alpha"]), std::stoi(args["beta"]));
	const auto end = std::chrono::system_clock::now();
	const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "finish: total elapsed time = " << elapsed << " ms" << std::endl;

	return 0;
}