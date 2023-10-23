
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


namespace std {
template <>
struct hash<std::array<uint64_t, 2>> {
	size_t operator()(const std::array<uint64_t, 2> &x) const {
		return get_hash_code(x[0], x[1]);
	}
};
}





constexpr int BUFSIZE = 2 * 1024 * 1024;

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

int main(int argc, char **argv) {

	init_hash_rank();
	init_eval_json("edax_eval_weight.json");

	std::vector<std::string>filenames;
	for(const auto & filename : std::filesystem::directory_iterator(".")) {
		const std::string filepath = filename.path().string();
		if (std::regex_search(filepath, std::regex(R"(result_[-OX]{64}_abtree\.csv)"))){
			filenames.push_back(filepath);
		}
	}
	if (filenames.size() != 2587){
		//error. exit.
		std::cout << "error: the number of result files is invalid. num=" << filenames.size() << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::vector<std::vector<std::pair<std::array<uint64_t, 2>, int64_t>>>searched_data_tmp(filenames.size());
	int count = 0;
#pragma omp parallel for schedule(dynamic, 1)
	for(int i = 0; i < filenames.size(); ++i){
#pragma omp critical
{
		std::cout << filenames[i] << " " << count++ << "/" << filenames.size() << std::endl;
}
		std::ifstream fin;
		fin.open(filenames[i]);
		if (!fin) {
			std::cout << "error: we could not read " << filenames[i] << std::endl;
			std::exit(EXIT_FAILURE);
		}
		std::string line;
		while (std::getline(fin, line)) {
			// if (!std::regex_search(line, std::regex(R"([-OX]{64}\s[OX];,-?[0-9]+,-?[0-9]+,-?[0-9]+)"))){
			// 	continue;
			// }
			std::vector<std::string> x = split(line, ',');
			if (x.size() != 4) {
				continue;
			}
			if (IsObfFormat(x[0]) == false) {
				continue;
			}
			if (x[3].size() == 0) {
				continue;
			}
			const int64_t num = std::stoll(x[3]);
			if(num == 0){
				continue;
			}
			const auto BB = board_unique(x[0]);
			searched_data_tmp[i].push_back(std::make_pair(BB, num));
			// if(searched_data.find(BB) == searched_data.end()){
			// 	searched_data[BB] = num;
			// }
			// else{
			// 	if(searched_data[BB] > 0){
			// 		if(num != searched_data[BB]){
			// 			std::cout << "error: the number of searched nodes is invalid. num != searched_data[BB]" << filename << std::endl;
			// 			std::exit(EXIT_FAILURE);
			// 		}
			// 	}
			// 	else{
			// 		searched_data[BB] = std::min(searched_data[BB], num);
			// 	}
			// }
		}
	}
	std::vector<std::pair<std::array<uint64_t, 2>, int64_t>>searched_data;
	for (int i = 0; i < searched_data_tmp.size(); ++i) {
		for (const auto &y : searched_data_tmp[i]) {
			searched_data.push_back(y);
		}
		searched_data_tmp[i].clear();
		searched_data_tmp[i].shrink_to_fit();
	}

	std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
	std::cout << "phase 1" << std::endl;
	{
		std::sort(searched_data.begin(), searched_data.end(), [](const std::pair<std::array<uint64_t, 2>, int64_t> &a, const std::pair<std::array<uint64_t, 2>, int64_t> &b) {
			if(a.first[0] != b.first[0])return a.first[0] < b.first[0];
			if(a.first[1] != b.first[1])return a.first[1] < b.first[1];
			return a.second < b.second;
		});
		const auto result = std::unique(searched_data.begin(), searched_data.end(), [](const std::pair<std::array<uint64_t, 2>, int64_t> &a, const std::pair<std::array<uint64_t, 2>, int64_t> &b) {
			return a.first[0] == b.first[0] && a.first[1] == b.first[1] && a.second == b.second;
		});
		searched_data.erase(result, searched_data.end());
	}
	std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
	std::cout << "phase 2" << std::endl;

	assert(searched_data[0].first[0] < searched_data[searched_data.size() - 1].first[0]);
	bool unique_flag = true;
	for(int i = 0; i < searched_data.size() - 1; ++i){
		if(searched_data[i].first[0] == searched_data[i + 1].first[0] && searched_data[i].first[1] == searched_data[i + 1].first[1]){
			unique_flag = false;
			break;
		}
	}
	if(unique_flag){

		std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
		std::cout << "skip phase 3, 4, 5, and 6" << std::endl;

	}
	else{

		for(int i = 0; i < searched_data.size() - 1; ++i){
			if(searched_data[i].first[0] == searched_data[i + 1].first[0] && searched_data[i].first[1] == searched_data[i + 1].first[1]){
				if(searched_data[i].second < 0 && searched_data[i + 1].second < 0){
					const auto m = std::min(searched_data[i].second, searched_data[i + 1].second);
					searched_data[i].second = m;
					searched_data[i + 1].second = m;
				}
			}
		}

		std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
		std::cout << "phase 3" << std::endl;
		{
			std::sort(searched_data.begin(), searched_data.end(), [](const std::pair<std::array<uint64_t, 2>, int64_t> &a, const std::pair<std::array<uint64_t, 2>, int64_t> &b) {
				if(a.first[0] != b.first[0])return a.first[0] < b.first[0];
				if(a.first[1] != b.first[1])return a.first[1] < b.first[1];
				return a.second < b.second;
			});
			const auto result = std::unique(searched_data.begin(), searched_data.end(), [](const std::pair<std::array<uint64_t, 2>, int64_t> &a, const std::pair<std::array<uint64_t, 2>, int64_t> &b) {
				return a.first[0] == b.first[0] && a.first[1] == b.first[1] && a.second == b.second;
			});
			searched_data.erase(result, searched_data.end());
		}
		
		std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
		std::cout << "phase 4" << std::endl;

		for(int i = 0; i < searched_data.size() - 1; ++i){
			if(searched_data[i].first[0] == searched_data[i + 1].first[0] && searched_data[i].first[1] == searched_data[i + 1].first[1]){
				assert(searched_data[i].second < 0 && 0 < searched_data[i + 1].second);
				searched_data[i].second = searched_data[i + 1].second;
			}
		}

		std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
		std::cout << "phase 5" << std::endl;
		{
			std::sort(searched_data.begin(), searched_data.end(), [](const std::pair<std::array<uint64_t, 2>, int64_t> &a, const std::pair<std::array<uint64_t, 2>, int64_t> &b) {
				if(a.first[0] != b.first[0])return a.first[0] < b.first[0];
				if(a.first[1] != b.first[1])return a.first[1] < b.first[1];
				return a.second < b.second;
			});
			const auto result = std::unique(searched_data.begin(), searched_data.end(), [](const std::pair<std::array<uint64_t, 2>, int64_t> &a, const std::pair<std::array<uint64_t, 2>, int64_t> &b) {
				return a.first[0] == b.first[0] && a.first[1] == b.first[1] && a.second == b.second;
			});
			searched_data.erase(result, searched_data.end());
		}
		std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
		std::cout << "phase 6" << std::endl;
	}

	std::vector<int32_t>vec_eval(searched_data.size(), 0);
#pragma omp parallel for schedule(guided)
	for(int i = 0; i < searched_data.size(); ++i){
		const auto BB = searched_data[i].first;
		vec_eval[i] = static_evaluation_function_without_override(BB);
	}

	std::cout << "searched_data.size() = " << searched_data.size() << std::endl;
	std::cout << "phase 7" << std::endl;
	static char buf_filesystem[BUFSIZE];//大きいのでスタック領域に置きたくないからstatic。（べつにmallocでもstd::vectorでもいいんだけど）

	std::ofstream writing_file;
	writing_file.rdbuf()->pubsetbuf(buf_filesystem, BUFSIZE);
	writing_file.open(std::string("all_result_abtree_staticeval.csv"), std::ios::out);

	for(int i = 0; i < searched_data.size(); ++i){
		const auto BB = searched_data[i].first;
		const int64_t num = searched_data[i].second;
		const int32_t eval = vec_eval[i];
		writing_file << bitboard2obf(BB[0], BB[1]) << "," << num << "," << eval << std::endl;
	}

	writing_file.close();

	return 0;
}