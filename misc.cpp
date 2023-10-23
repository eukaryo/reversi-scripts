
#include "Header.hpp"

std::string bitboard2obf(const uint64_t player, const uint64_t opponent) {

	std::string s = "";
	for (int i = 0; i < 64; ++i) {
		if (player & (1ULL << i)) {
			s += "X";
		}
		else if (opponent & (1ULL << i)) {
			s += "O";
		}
		else {
			s += "-";
		}
	}

	s += " X;";
	return s;
}
void obf2bitboard(const std::string &obf, uint64_t &player, uint64_t &opponent) {

	//assert(std::regex_match(obf, std::regex(R"(^[-OX]{64}\s[XO];$)")));
	assert(IsObfFormat(obf));

	player = 0;
	opponent = 0;
	for (int i = 0; i < 64; ++i) {
		if (obf[i] == obf[65])player += 1ULL << i;
		else if (obf[i] != '-')opponent += 1ULL << i;
	}
}
static uint64_t transpose(uint64_t b) {
	uint64_t t;

	t = (b ^ (b >> 7)) & 0x00AA00AA00AA00AAULL;
	b = b ^ t ^ (t << 7);
	t = (b ^ (b >> 14)) & 0x0000CCCC0000CCCCULL;
	b = b ^ t ^ (t << 14);
	t = (b ^ (b >> 28)) & 0x00000000F0F0F0F0ULL;
	b = b ^ t ^ (t << 28);

	return b;
}
static uint64_t vertical_mirror(uint64_t b) {
	b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b << 8) & 0xFF00FF00FF00FF00ULL);
	b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b << 16) & 0xFFFF0000FFFF0000ULL);
	b = ((b >> 32) & 0x00000000FFFFFFFFULL) | ((b << 32) & 0xFFFFFFFF00000000ULL);
	return b;
}
static uint64_t horizontal_mirror(uint64_t b) {
	b = ((b >> 1) & 0x5555555555555555ULL) | ((b << 1) & 0xAAAAAAAAAAAAAAAAULL);
	b = ((b >> 2) & 0x3333333333333333ULL) | ((b << 2) & 0xCCCCCCCCCCCCCCCCULL);
	b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b << 4) & 0xF0F0F0F0F0F0F0F0ULL);

	return b;
}
static void board_check(const uint64_t player, const uint64_t opponent)
{
	if (player & opponent) {
		assert(0);
	}

	if (((player | opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) {
		assert(0);
	}
}
static int board_compare(const uint64_t p0, const uint64_t o0, const uint64_t p1, const uint64_t o1) {
	if (p0 > p1) return 1;
	else if (p0 < p1) return -1;
	else if (o0 > o1) return 1;
	else if (o0 < o1) return -1;
	else return 0;
}
static void board_symetry(const int s, const uint64_t original_player, const uint64_t original_opponent, uint64_t &player, uint64_t &opponent) {

	player = original_player;
	opponent = original_opponent;

	if (s & 1) {
		player = horizontal_mirror(player);
		opponent = horizontal_mirror(opponent);
	}
	if (s & 2) {
		player = vertical_mirror(player);
		opponent = vertical_mirror(opponent);
	}
	if (s & 4) {
		player = transpose(player);
		opponent = transpose(opponent);
	}
}
static void board_unique_naive(const uint64_t original_player, const uint64_t original_opponent, uint64_t &player, uint64_t &opponent) {

	board_check(original_player, original_opponent);

	uint64_t tmp_player = original_player;
	uint64_t tmp_opponent = original_opponent;
	player = original_player;
	opponent = original_opponent;
	int s = 0;

	for (int i = 1; i < 8; ++i) {
		board_symetry(i, original_player, original_opponent, tmp_player, tmp_opponent);
		if (board_compare(tmp_player, tmp_opponent, player, opponent) < 0) {
			player = tmp_player;
			opponent = tmp_opponent;
			s = i;
		}
	}

	board_check(player, opponent);
}
void board_unique(const uint64_t P_src, const uint64_t O_src, uint64_t &P_dest, uint64_t &O_dest)
{

	const __m256i bb0_ppoo = _mm256_set_epi64x(P_src, P_src, O_src, O_src);

	const __m256i tt1lo_ppoo = _mm256_and_si256(_mm256_srlv_epi64(bb0_ppoo, _mm256_set_epi64x(1, 8, 1, 8)), _mm256_set_epi64x(0x5555555555555555LL, 0x00FF00FF00FF00FFLL, 0x5555555555555555LL, 0x00FF00FF00FF00FFLL));
	const __m256i tt1hi_ppoo = _mm256_and_si256(_mm256_sllv_epi64(bb0_ppoo, _mm256_set_epi64x(1, 8, 1, 8)), _mm256_set_epi64x(0xAAAAAAAAAAAAAAAALL, 0xFF00FF00FF00FF00LL, 0xAAAAAAAAAAAAAAAALL, 0xFF00FF00FF00FF00LL));
	const __m256i tt1_ppoo = _mm256_or_si256(tt1lo_ppoo, tt1hi_ppoo);

	const __m256i tt2lo_ppoo = _mm256_and_si256(_mm256_srlv_epi64(tt1_ppoo, _mm256_set_epi64x(2, 16, 2, 16)), _mm256_set_epi64x(0x3333333333333333LL, 0x0000FFFF0000FFFFLL, 0x3333333333333333LL, 0x0000FFFF0000FFFFLL));
	const __m256i tt2hi_ppoo = _mm256_and_si256(_mm256_sllv_epi64(tt1_ppoo, _mm256_set_epi64x(2, 16, 2, 16)), _mm256_set_epi64x(0xCCCCCCCCCCCCCCCCLL, 0xFFFF0000FFFF0000LL, 0xCCCCCCCCCCCCCCCCLL, 0xFFFF0000FFFF0000LL));
	const __m256i tt2_ppoo = _mm256_or_si256(tt2lo_ppoo, tt2hi_ppoo);

	const __m256i tt3lo_ppoo = _mm256_and_si256(_mm256_srlv_epi64(tt2_ppoo, _mm256_set_epi64x(4, 32, 4, 32)), _mm256_set_epi64x(0x0F0F0F0F0F0F0F0FLL, 0x00000000FFFFFFFFLL, 0x0F0F0F0F0F0F0F0FLL, 0x00000000FFFFFFFFLL));
	const __m256i tt3hi_ppoo = _mm256_and_si256(_mm256_sllv_epi64(tt2_ppoo, _mm256_set_epi64x(4, 32, 4, 32)), _mm256_set_epi64x(0xF0F0F0F0F0F0F0F0LL, 0xFFFFFFFF00000000LL, 0xF0F0F0F0F0F0F0F0LL, 0xFFFFFFFF00000000LL));
	const __m256i tt3_ppoo = _mm256_or_si256(tt3lo_ppoo, tt3hi_ppoo);//この時点で、上位から順に(P横鏡映、P縦鏡映、O横鏡映、O縦鏡映)

	constexpr auto f = [](const uint8_t i) {
		return uint8_t(((i & 1) << 3) + ((i & 2) << 1) + ((i & 4) >> 1) + ((i & 8) >> 3));
	};

	const __m256i rvr1 = _mm256_set_epi8(
		f(15), f(14), f(13), f(12), f(11), f(10), f(9), f(8), f(7), f(6), f(5), f(4), f(3), f(2), f(1), f(0),
		f(15), f(14), f(13), f(12), f(11), f(10), f(9), f(8), f(7), f(6), f(5), f(4), f(3), f(2), f(1), f(0));

	const __m256i rva1 = _mm256_set_epi64x(
		(P_src >> 4) & 0x0F0F'0F0F'0F0F'0F0FULL, P_src & 0x0F0F'0F0F'0F0F'0F0FULL,
		(O_src >> 4) & 0x0F0F'0F0F'0F0F'0F0FULL, O_src & 0x0F0F'0F0F'0F0F'0F0FULL);
	const __m256i rva2 = _mm256_shuffle_epi8(rvr1, rva1);
	const __m256i rva3 = _mm256_shuffle_epi8(rva2, _mm256_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7));
	const __m256i rva4 = _mm256_shuffle_epi32(rva3, 0b00001110);
	const __m256i rva5 = _mm256_add_epi32(rva4, _mm256_slli_epi64(rva3, 4));//この時点で、上位から順に(any、P逆転、any、O逆転)
	const __m256i rev_ppoo = _mm256_blend_epi32(rva5, bb0_ppoo, 0b11001100);//この時点で、上位から順に(Pそのまま、P逆転、Oそのまま、O逆転)

	const __m256i x1_t = _mm256_and_si256(_mm256_xor_si256(tt3_ppoo, _mm256_srli_epi64(tt3_ppoo, 7)), _mm256_set1_epi64x(0x00AA00AA00AA00AALL));
	const __m256i x1_r = _mm256_and_si256(_mm256_xor_si256(rev_ppoo, _mm256_srli_epi64(rev_ppoo, 7)), _mm256_set1_epi64x(0x00AA00AA00AA00AALL));
	const __m256i y1_t = _mm256_xor_si256(tt3_ppoo, _mm256_xor_si256(x1_t, _mm256_slli_epi64(x1_t, 7)));
	const __m256i y1_r = _mm256_xor_si256(rev_ppoo, _mm256_xor_si256(x1_r, _mm256_slli_epi64(x1_r, 7)));
	const __m256i x2_t = _mm256_and_si256(_mm256_xor_si256(y1_t, _mm256_srli_epi64(y1_t, 14)), _mm256_set1_epi64x(0x0000CCCC0000CCCCLL));
	const __m256i x2_r = _mm256_and_si256(_mm256_xor_si256(y1_r, _mm256_srli_epi64(y1_r, 14)), _mm256_set1_epi64x(0x0000CCCC0000CCCCLL));
	const __m256i y2_t = _mm256_xor_si256(y1_t, _mm256_xor_si256(x2_t, _mm256_slli_epi64(x2_t, 14)));
	const __m256i y2_r = _mm256_xor_si256(y1_r, _mm256_xor_si256(x2_r, _mm256_slli_epi64(x2_r, 14)));
	const __m256i x3_t = _mm256_and_si256(_mm256_xor_si256(y2_t, _mm256_srli_epi64(y2_t, 28)), _mm256_set1_epi64x(0x00000000F0F0F0F0LL));
	const __m256i x3_r = _mm256_and_si256(_mm256_xor_si256(y2_r, _mm256_srli_epi64(y2_r, 28)), _mm256_set1_epi64x(0x00000000F0F0F0F0LL));
	const __m256i zz_t = _mm256_xor_si256(y2_t, _mm256_xor_si256(x3_t, _mm256_slli_epi64(x3_t, 28)));//この時点で、tt3_ppooの各要素を行列転置したもの
	const __m256i zz_r = _mm256_xor_si256(y2_r, _mm256_xor_si256(x3_r, _mm256_slli_epi64(x3_r, 28)));//この時点で、rev_ppooの各要素を行列転置したもの

	alignas(32) uint64_t bb[16] = {};
	_mm256_storeu_si256((__m256i*)(&(bb[0])), tt3_ppoo);
	_mm256_storeu_si256((__m256i*)(&(bb[4])), rev_ppoo);
	_mm256_storeu_si256((__m256i*)(&(bb[8])), zz_t);
	_mm256_storeu_si256((__m256i*)(&(bb[12])), zz_r);

	//uint64_t pbb[8] = { bb[2],bb[3],bb[6],bb[7],bb[10],bb[11],bb[14],bb[15] };
	//uint64_t obb[8] = { bb[0],bb[1],bb[4],bb[5],bb[8],bb[9],bb[12],bb[13] };

	//P_dest = bb[2];
	//O_dest = bb[0];

	{ //for(uint64_t i = 0; i < 16; i += 4) 
		{
			constexpr int i = 0;
			constexpr int p_index = i + 2;
			constexpr int o_index = i;

			const bool f = bool(bb[p_index] < bb[p_index + 1]) | (bool(bb[p_index] == bb[p_index + 1]) & bool(bb[o_index] < bb[o_index + 1]));
			bb[p_index] = f ? bb[p_index] : bb[p_index + 1];
			bb[o_index] = f ? bb[o_index] : bb[o_index + 1];
		}
		{
			constexpr int i = 4;
			constexpr int p_index = i + 2;
			constexpr int o_index = i;

			const bool f = bool(bb[p_index] < bb[p_index + 1]) | (bool(bb[p_index] == bb[p_index + 1]) & bool(bb[o_index] < bb[o_index + 1]));
			bb[p_index] = f ? bb[p_index] : bb[p_index + 1];
			bb[o_index] = f ? bb[o_index] : bb[o_index + 1];
		}
		{
			constexpr int i = 8;
			constexpr int p_index = i + 2;
			constexpr int o_index = i;

			const bool f = bool(bb[p_index] < bb[p_index + 1]) | (bool(bb[p_index] == bb[p_index + 1]) & bool(bb[o_index] < bb[o_index + 1]));
			bb[p_index] = f ? bb[p_index] : bb[p_index + 1];
			bb[o_index] = f ? bb[o_index] : bb[o_index + 1];
		}
		{
			constexpr int i = 12;
			constexpr int p_index = i + 2;
			constexpr int o_index = i;

			const bool f = bool(bb[p_index] < bb[p_index + 1]) | (bool(bb[p_index] == bb[p_index + 1]) & bool(bb[o_index] < bb[o_index + 1]));
			bb[p_index] = f ? bb[p_index] : bb[p_index + 1];
			bb[o_index] = f ? bb[o_index] : bb[o_index + 1];
		}
	}
	{ //for(uint64_t i = 0; i < 16; i += 8) 
		{
			constexpr int i = 0;
			constexpr int p_index = i + 2;
			constexpr int o_index = i;

			const bool f = bool(bb[p_index] < bb[p_index + 4]) | (bool(bb[p_index] == bb[p_index + 4]) & bool(bb[o_index] < bb[o_index + 4]));
			bb[p_index] = f ? bb[p_index] : bb[p_index + 4];
			bb[o_index] = f ? bb[o_index] : bb[o_index + 4];
		}
		{
			constexpr int i = 8;
			constexpr int p_index = i + 2;
			constexpr int o_index = i;

			const bool f = bool(bb[p_index] < bb[p_index + 4]) | (bool(bb[p_index] == bb[p_index + 4]) & bool(bb[o_index] < bb[o_index + 4]));
			bb[p_index] = f ? bb[p_index] : bb[p_index + 4];
			bb[o_index] = f ? bb[o_index] : bb[o_index + 4];
		}
	}
	{
		constexpr int i = 0;
		constexpr int p_index = i + 2;
		constexpr int o_index = i;

		const bool f = bool(bb[p_index] < bb[p_index + 8]) | (bool(bb[p_index] == bb[p_index + 8]) & bool(bb[o_index] < bb[o_index + 8]));
		P_dest = f ? bb[p_index] : bb[p_index + 8];
		O_dest = f ? bb[o_index] : bb[o_index + 8];
	}
}

std::array<uint64_t, 2>board_unique(const uint64_t original_player, const uint64_t original_opponent) {
	std::array<uint64_t, 2> answer;
	board_unique(original_player, original_opponent, answer[0], answer[1]);
	return answer;
}
std::array<uint64_t, 2>board_unique(const std::array<uint64_t, 2> original_bitboards) {
	std::array<uint64_t, 2> answer;
	board_unique(original_bitboards[0], original_bitboards[1], answer[0], answer[1]);
	return answer;
}
std::array<uint64_t, 2>board_unique(const std::string &obf) {
	uint64_t player = 0, opponent = 0;
	obf2bitboard(obf, player, opponent);
	return board_unique(player, opponent);
}
bool IsObfFormat(const std::string &s) {

	//return std::regex_match(x[0], std::regex(R"(^[-OX]{64}\s[XO];$)"));

	//上記と等価な計算を行う。

	if (s.size() != 67)return false;

	for (int i = 0; i < 64; ++i) {
		if (s[i] != 'X' && s[i] != 'O' && s[i] != '-')return false;
	}
	if (s[64] != ' ')return false;
	if (s[65] != 'X' && s[65] != 'O')return false;
	if (s[66] != ';')return false;
	return true;
}
uint64_t HorizontalOr(const __m256i x) {
	const __m128i y = _mm_or_si128(_mm256_castsi256_si128(x), _mm256_extracti128_si256(x, 1));
	return uint64_t(_mm_cvtsi128_si64(y) | _mm_extract_epi64(y, 1));
}







static uint64_t hash_rank[16][256];

void init_hash_rank() {
	std::mt19937_64 rnd(123456789);

	for (int i = 0; i < 16; ++i) for (int j = 0; j < 256; ++j) {
		for (;;) {
			hash_rank[i][j] = rnd();
			if (8 <= _mm_popcnt_u64(hash_rank[i][j]) && _mm_popcnt_u64(hash_rank[i][j]) <= 56)break;
		}
	}
}

uint64_t get_hash_code(uint64_t player, uint64_t opponent) {

	uint64_t h1 = hash_rank[0][player % 256];
	uint64_t h2 = hash_rank[1][opponent % 256];

	for (int i = 2; i < 14; i += 2) {
		player /= 256;
		opponent /= 256;
		h1 ^= hash_rank[i + 0][player % 256];
		h2 ^= hash_rank[i + 1][opponent % 256];
	}

	//h1 ^= hash_rank[2][(player >> 8) % 256];
	//h2 ^= hash_rank[3][(opponent >> 8) % 256];
	//h1 ^= hash_rank[4][(player >> 16) % 256];
	//h2 ^= hash_rank[5][(opponent >> 16) % 256];
	//h1 ^= hash_rank[6][(player >> 24) % 256];
	//h2 ^= hash_rank[7][(opponent >> 24) % 256];
	//h1 ^= hash_rank[8][(player >> 32) % 256];
	//h2 ^= hash_rank[9][(opponent >> 32) % 256];
	//h1 ^= hash_rank[10][(player >> 40) % 256];
	//h2 ^= hash_rank[11][(opponent >> 40) % 256];
	//h1 ^= hash_rank[12][(player >> 48) % 256];
	//h2 ^= hash_rank[13][(opponent >> 48) % 256];

	h1 ^= hash_rank[14][player >> 56];
	h2 ^= hash_rank[15][opponent >> 56];

	return h1 ^ h2;
}

static uint64_t flip_naive(const uint64_t square, const uint64_t player, const uint64_t opponent)
{
	const int dir[8] = { -9,-8,-7,-1,1,7,8,9 };
	const uint64_t edge[8] = {
		0x01010101010101ffull,
		0x00000000000000ffull,
		0x80808080808080ffull,
		0x0101010101010101ull,
		0x8080808080808080ull,
		0xff01010101010101ull,
		0xff00000000000000ull,
		0xff80808080808080ull
	};

	assert(square < 64);

	uint64_t flipped = 0;
	for (int d = 0; d < 8; ++d) {
		if (((1ULL << square) & edge[d]) == 0) {
			uint64_t f = 0;
			int x = square + dir[d];
			for (; (opponent & (1ULL << x)) && ((1ULL << x) & edge[d]) == 0; x += dir[d]) {
				f |= (1ULL << x);
			}
			if (player & (1ULL << x)) flipped |= f;
		}
	}
	return flipped;
}

alignas(64) static const uint64_t LEFT_MASK_256[64][4] = {
	{ 0x00000000000000fe, 0x0101010101010100, 0x8040201008040200, 0x0000000000000000 },
	{ 0x00000000000000fc, 0x0202020202020200, 0x0080402010080400, 0x0000000000000100 },
	{ 0x00000000000000f8, 0x0404040404040400, 0x0000804020100800, 0x0000000000010200 },
	{ 0x00000000000000f0, 0x0808080808080800, 0x0000008040201000, 0x0000000001020400 },
	{ 0x00000000000000e0, 0x1010101010101000, 0x0000000080402000, 0x0000000102040800 },
	{ 0x00000000000000c0, 0x2020202020202000, 0x0000000000804000, 0x0000010204081000 },
	{ 0x0000000000000080, 0x4040404040404000, 0x0000000000008000, 0x0001020408102000 },
	{ 0x0000000000000000, 0x8080808080808000, 0x0000000000000000, 0x0102040810204000 },
	{ 0x000000000000fe00, 0x0101010101010000, 0x4020100804020000, 0x0000000000000000 },
	{ 0x000000000000fc00, 0x0202020202020000, 0x8040201008040000, 0x0000000000010000 },
	{ 0x000000000000f800, 0x0404040404040000, 0x0080402010080000, 0x0000000001020000 },
	{ 0x000000000000f000, 0x0808080808080000, 0x0000804020100000, 0x0000000102040000 },
	{ 0x000000000000e000, 0x1010101010100000, 0x0000008040200000, 0x0000010204080000 },
	{ 0x000000000000c000, 0x2020202020200000, 0x0000000080400000, 0x0001020408100000 },
	{ 0x0000000000008000, 0x4040404040400000, 0x0000000000800000, 0x0102040810200000 },
	{ 0x0000000000000000, 0x8080808080800000, 0x0000000000000000, 0x0204081020400000 },
	{ 0x0000000000fe0000, 0x0101010101000000, 0x2010080402000000, 0x0000000000000000 },
	{ 0x0000000000fc0000, 0x0202020202000000, 0x4020100804000000, 0x0000000001000000 },
	{ 0x0000000000f80000, 0x0404040404000000, 0x8040201008000000, 0x0000000102000000 },
	{ 0x0000000000f00000, 0x0808080808000000, 0x0080402010000000, 0x0000010204000000 },
	{ 0x0000000000e00000, 0x1010101010000000, 0x0000804020000000, 0x0001020408000000 },
	{ 0x0000000000c00000, 0x2020202020000000, 0x0000008040000000, 0x0102040810000000 },
	{ 0x0000000000800000, 0x4040404040000000, 0x0000000080000000, 0x0204081020000000 },
	{ 0x0000000000000000, 0x8080808080000000, 0x0000000000000000, 0x0408102040000000 },
	{ 0x00000000fe000000, 0x0101010100000000, 0x1008040200000000, 0x0000000000000000 },
	{ 0x00000000fc000000, 0x0202020200000000, 0x2010080400000000, 0x0000000100000000 },
	{ 0x00000000f8000000, 0x0404040400000000, 0x4020100800000000, 0x0000010200000000 },
	{ 0x00000000f0000000, 0x0808080800000000, 0x8040201000000000, 0x0001020400000000 },
	{ 0x00000000e0000000, 0x1010101000000000, 0x0080402000000000, 0x0102040800000000 },
	{ 0x00000000c0000000, 0x2020202000000000, 0x0000804000000000, 0x0204081000000000 },
	{ 0x0000000080000000, 0x4040404000000000, 0x0000008000000000, 0x0408102000000000 },
	{ 0x0000000000000000, 0x8080808000000000, 0x0000000000000000, 0x0810204000000000 },
	{ 0x000000fe00000000, 0x0101010000000000, 0x0804020000000000, 0x0000000000000000 },
	{ 0x000000fc00000000, 0x0202020000000000, 0x1008040000000000, 0x0000010000000000 },
	{ 0x000000f800000000, 0x0404040000000000, 0x2010080000000000, 0x0001020000000000 },
	{ 0x000000f000000000, 0x0808080000000000, 0x4020100000000000, 0x0102040000000000 },
	{ 0x000000e000000000, 0x1010100000000000, 0x8040200000000000, 0x0204080000000000 },
	{ 0x000000c000000000, 0x2020200000000000, 0x0080400000000000, 0x0408100000000000 },
	{ 0x0000008000000000, 0x4040400000000000, 0x0000800000000000, 0x0810200000000000 },
	{ 0x0000000000000000, 0x8080800000000000, 0x0000000000000000, 0x1020400000000000 },
	{ 0x0000fe0000000000, 0x0101000000000000, 0x0402000000000000, 0x0000000000000000 },
	{ 0x0000fc0000000000, 0x0202000000000000, 0x0804000000000000, 0x0001000000000000 },
	{ 0x0000f80000000000, 0x0404000000000000, 0x1008000000000000, 0x0102000000000000 },
	{ 0x0000f00000000000, 0x0808000000000000, 0x2010000000000000, 0x0204000000000000 },
	{ 0x0000e00000000000, 0x1010000000000000, 0x4020000000000000, 0x0408000000000000 },
	{ 0x0000c00000000000, 0x2020000000000000, 0x8040000000000000, 0x0810000000000000 },
	{ 0x0000800000000000, 0x4040000000000000, 0x0080000000000000, 0x1020000000000000 },
	{ 0x0000000000000000, 0x8080000000000000, 0x0000000000000000, 0x2040000000000000 },
	{ 0x00fe000000000000, 0x0100000000000000, 0x0200000000000000, 0x0000000000000000 },
	{ 0x00fc000000000000, 0x0200000000000000, 0x0400000000000000, 0x0100000000000000 },
	{ 0x00f8000000000000, 0x0400000000000000, 0x0800000000000000, 0x0200000000000000 },
	{ 0x00f0000000000000, 0x0800000000000000, 0x1000000000000000, 0x0400000000000000 },
	{ 0x00e0000000000000, 0x1000000000000000, 0x2000000000000000, 0x0800000000000000 },
	{ 0x00c0000000000000, 0x2000000000000000, 0x4000000000000000, 0x1000000000000000 },
	{ 0x0080000000000000, 0x4000000000000000, 0x8000000000000000, 0x2000000000000000 },
	{ 0x0000000000000000, 0x8000000000000000, 0x0000000000000000, 0x4000000000000000 },
	{ 0xfe00000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xfc00000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xf800000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xf000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xe000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0xc000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0x8000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
	{ 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 },
};

uint64_t flip(const uint64_t square, const uint64_t player, const uint64_t opponent) {

	assert(square < 64);

	__m256i	flip, outflank, ocontig;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	const __m256i shift1897_2 = _mm256_set_epi64x(14, 18, 16, 2);
	const __m256i mask_flip1897 = _mm256_set_epi64x(0x007e7e7e7e7e7e00, 0x007e7e7e7e7e7e00, 0x00ffffffffffff00, 0x7e7e7e7e7e7e7e7e);

	const __m256i PPPP = _mm256_set1_epi64x(player);
	const __m256i OOOO = _mm256_set1_epi64x(opponent);
	const __m256i OOOO_masked = _mm256_and_si256(OOOO, mask_flip1897);
	const __m256i pre = _mm256_and_si256(OOOO_masked, _mm256_srlv_epi64(OOOO_masked, shift1897));
	const __m256i mask = _mm256_loadu_si256((__m256i*)LEFT_MASK_256[square]);

	ocontig = _mm256_set1_epi64x(1ULL << square);

	ocontig = _mm256_and_si256(OOOO_masked, _mm256_srlv_epi64(ocontig, shift1897));
	ocontig = _mm256_or_si256(ocontig, _mm256_and_si256(OOOO_masked, _mm256_srlv_epi64(ocontig, shift1897)));
	ocontig = _mm256_or_si256(ocontig, _mm256_and_si256(pre, _mm256_srlv_epi64(ocontig, shift1897_2)));
	ocontig = _mm256_or_si256(ocontig, _mm256_and_si256(pre, _mm256_srlv_epi64(ocontig, shift1897_2)));
	outflank = _mm256_and_si256(_mm256_srlv_epi64(ocontig, shift1897), PPPP);
	flip = _mm256_andnot_si256(_mm256_cmpeq_epi64(outflank, _mm256_setzero_si256()), ocontig);

	ocontig = _mm256_andnot_si256(OOOO_masked, mask);
	ocontig = _mm256_and_si256(ocontig, _mm256_sub_epi64(_mm256_setzero_si256(), ocontig));	// LS1B
	outflank = _mm256_and_si256(ocontig, PPPP);
	flip = _mm256_or_si256(flip, _mm256_and_si256(mask, _mm256_add_epi64(outflank, _mm256_cmpeq_epi64(outflank, ocontig))));

	const uint64_t answer = HorizontalOr(flip);
	
	//assert(answer == flip_naive(square, player, opponent));

	return answer;
}

uint64_t get_moves_naive(const uint64_t player, const uint64_t opponent) {

	const auto get_some_moves = [](const uint64_t P, const uint64_t mask, const int dir)
	{
		uint64_t flip;

		flip = (((P << dir) | (P >> dir)) & mask);
		flip |= (((flip << dir) | (flip >> dir)) & mask);
		flip |= (((flip << dir) | (flip >> dir)) & mask);
		flip |= (((flip << dir) | (flip >> dir)) & mask);
		flip |= (((flip << dir) | (flip >> dir)) & mask);
		flip |= (((flip << dir) | (flip >> dir)) & mask);
		return (flip << dir) | (flip >> dir);
	};

	const uint64_t mask = opponent & 0x7E7E7E7E7E7E7E7Eull;

	return (get_some_moves(player, mask, 1) // horizontal
		| get_some_moves(player, opponent, 8)   // vertical
		| get_some_moves(player, mask, 7)   // diagonals
		| get_some_moves(player, mask, 9))
		& ~(player | opponent); // mask with empties
}

uint64_t get_moves(const uint64_t P, const uint64_t O) {

	__m256i	flip_l, flip_r, pre_l, pre_r, shift2;
	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	const __m256i mflipH = _mm256_set_epi64x(0x7e7e7e7e7e7e7e7e, 0x7e7e7e7e7e7e7e7e, -1, 0x7e7e7e7e7e7e7e7e);

	const __m256i PPPP = _mm256_set1_epi64x(P);
	const __m256i mOOOO = _mm256_and_si256(_mm256_set1_epi64x(O), mflipH);

	flip_l = _mm256_and_si256(mOOOO, _mm256_sllv_epi64(PPPP, shift1897));
	flip_r = _mm256_and_si256(mOOOO, _mm256_srlv_epi64(PPPP, shift1897));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOOOO, _mm256_sllv_epi64(flip_l, shift1897)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOOOO, _mm256_srlv_epi64(flip_r, shift1897)));
	pre_l = _mm256_and_si256(mOOOO, _mm256_sllv_epi64(mOOOO, shift1897));
	pre_r = _mm256_srlv_epi64(pre_l, shift1897);
	shift2 = _mm256_add_epi64(shift1897, shift1897);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));

	const __m256i MMMM = _mm256_or_si256(_mm256_sllv_epi64(flip_l, shift1897), _mm256_srlv_epi64(flip_r, shift1897));

	return HorizontalOr(MMMM) & ~(P | O);	// mask with empties
}

static constexpr int32_t w_mobility = 1 << 15;
static constexpr int32_t w_corner_stability = 1 << 11;
static constexpr int32_t w_edge_stability = 1 << 11;
static constexpr int32_t w_potential_mobility = 1 << 5;

static inline uint64_t bit_weighted_count(const uint64_t b)
{
	return _mm_popcnt_u64(b) + _mm_popcnt_u64(b & 0x8100000000000081ULL);
}

static int32_t integrated_move_scoring_naive(const uint64_t player, const uint64_t opponent, const int32_t pos) {

	const auto get_corner_stability = [&](const uint64_t P)
	{
		const uint64_t stable =
			((((0x0100000000000001ULL & P) << 1)
				| ((0x8000000000000080ULL & P) >> 1)
				| ((0x0000000000000081ULL & P) << 8)
				| ((0x8100000000000000ULL & P) >> 8)
				| 0x8100000000000081ULL) & P);
		return _mm_popcnt_u64(stable);
	};

	const auto get_weighted_mobility = [&](const uint64_t P, const uint64_t O)
	{
		return bit_weighted_count(get_moves(P, O));
	};

	const auto get_some_potential_moves = [&](const uint64_t P, const int dir)
	{
		return (P << dir | P >> dir);
	};

	const auto get_potential_moves = [&](const uint64_t P, const uint64_t O)
	{
		return (get_some_potential_moves(O & 0x7E7E7E7E7E7E7E7Eull, 1) // horizontal
			| get_some_potential_moves(O & 0x00FFFFFFFFFFFF00ull, 8)   // vertical
			| get_some_potential_moves(O & 0x007E7E7E7E7E7E00ull, 7)   // diagonals
			| get_some_potential_moves(O & 0x007E7E7E7E7E7E00ull, 9))
			& ~(P | O); // mask with empties
	};

	const auto get_potential_mobility = [&](const uint64_t P, const uint64_t O)
	{
		return bit_weighted_count(get_potential_moves(P, O));
	};

	int32_t score = 0;
	score += (36 - get_potential_mobility(player, opponent)) * w_potential_mobility; // potential mobility
	score += get_corner_stability(opponent) * w_corner_stability; // corner stability
	score += (36 - get_weighted_mobility(player, opponent)) * w_mobility; // real mobility

	return score;
}

int32_t integrated_move_scoring(const uint64_t player, const uint64_t opponent, const int32_t pos) {

	//move->score += (36 - get_potential_mobility(board->player, board->opponent)) * w_potential_mobility; // potential mobility
	//move->score += get_corner_stability(board->opponent) * w_corner_stability; // corner stability
	//move->score += (36 - get_weighted_mobility(board->player, board->opponent)) * w_mobility; // real mobility

	uint64_t answer = 0;
	const uint64_t bb_empty = ~(player | opponent);

	const __m256i shift1897 = _mm256_set_epi64x(7, 9, 8, 1);
	const __m256i mflipH = _mm256_set_epi64x(0x7e7e7e7e7e7e7e7e, 0x7e7e7e7e7e7e7e7e, -1, 0x7e7e7e7e7e7e7e7e);
	const __m256i PPPP = _mm256_set1_epi64x(player);
	const __m256i OOOO = _mm256_set1_epi64x(opponent);
	const __m256i mOOOO = _mm256_and_si256(OOOO, mflipH);
	const __m256i mask_potential_mobility_1897 = _mm256_set_epi64x(0x007E7E7E7E7E7E00ull, 0x007E7E7E7E7E7E00ull, 0x00FFFFFFFFFFFF00ull, 0x7E7E7E7E7E7E7E7Eull);
	const __m256i shift1188 = _mm256_set_epi64x(8, 8, 1, 1);

	__m256i	flip_l, flip_r, pre_l, pre_r, shift2;

	flip_l = _mm256_and_si256(mOOOO, _mm256_sllv_epi64(PPPP, shift1897));
	flip_r = _mm256_and_si256(mOOOO, _mm256_srlv_epi64(PPPP, shift1897));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOOOO, _mm256_sllv_epi64(flip_l, shift1897)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOOOO, _mm256_srlv_epi64(flip_r, shift1897)));
	pre_l = _mm256_and_si256(mOOOO, _mm256_sllv_epi64(mOOOO, shift1897));
	pre_r = _mm256_srlv_epi64(pre_l, shift1897);
	shift2 = _mm256_add_epi64(shift1897, shift1897);
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
	flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
	flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));

	const __m256i MMMM = _mm256_or_si256(_mm256_sllv_epi64(flip_l, shift1897), _mm256_srlv_epi64(flip_r, shift1897));
	const uint64_t bb_moves = HorizontalOr(MMMM) & bb_empty;
	const uint64_t weighted_mobility = bit_weighted_count(bb_moves);
	answer += ((MAX_MOVE + 2) - weighted_mobility) * w_mobility;

	const __m256i mask_potential_OOOO = _mm256_and_si256(OOOO, mask_potential_mobility_1897);
	const __m256i TTTT = _mm256_or_si256(_mm256_sllv_epi64(mask_potential_OOOO, shift1897), _mm256_srlv_epi64(mask_potential_OOOO, shift1897));
	const uint64_t potential_moves = HorizontalOr(TTTT) & bb_empty;
	const uint64_t potential_mobility = bit_weighted_count(potential_moves);
	answer += ((MAX_MOVE + 2) - potential_mobility) * w_potential_mobility;

	const __m256i mask_corner_stable = _mm256_set_epi64x(0x0100000000000001ULL, 0x8000000000000080ULL, 0x0000000000000081ULL, 0x8100000000000000ULL);
	const __m256i shift_corner_stable = _mm256_set_epi64x(1, 1, 8, 8);
	const __m256i s1 = _mm256_and_si256(OOOO, mask_corner_stable);
	const __m256i SSSS = _mm256_blend_epi32(_mm256_sllv_epi64(s1, shift_corner_stable), _mm256_srlv_epi64(s1, shift_corner_stable), 0b00110011);
	const uint64_t corner_stable_discs = (HorizontalOr(SSSS) | 0x8100000000000081ULL) & opponent;
	const uint64_t corner_stability = _mm_popcnt_u64(corner_stable_discs);
	answer += corner_stability * w_corner_stability;

	return answer;
}

static uint64_t test_random_playout(const uint64_t player, const uint64_t opponent, std::mt19937_64 &rnd) {

	assert((player & opponent) == 0);
	const uint64_t n_empties = _mm_popcnt_u64(~(player | opponent));

	const auto score1 = EvaluatePosition0(player, opponent);
	const auto score2 = EvaluatePosition0_naive(player, opponent);
	assert(score1 == score2);
	assert(-64 <= score1 && score1 <= 64);

	uint64_t bb_moves = get_moves(player, opponent);

	const uint64_t bb_moves_naive = get_moves_naive(player, opponent);
	assert(bb_moves == bb_moves_naive);

	assert((bb_moves & (player | opponent)) == 0);

	if (bb_moves == 0) {
		if (get_moves(opponent, player) != 0) { // pass
			return test_random_playout(opponent, player, rnd);
		}
		return uint64_t(score1 + 65);
	}

	const int move_index = std::uniform_int_distribution<int>(0, _mm_popcnt_u64(bb_moves) - 1)(rnd);
	const uint64_t move_bit = pdep_intrinsics(1ULL << move_index, bb_moves);
	const uint64_t move_square = _mm_popcnt_u64(move_bit - 1);

	assert(_mm_popcnt_u64(move_bit) == 1);
	assert((bb_moves & move_bit) != 0);
	assert(_mm_popcnt_u64(bb_moves & (move_bit - 1ULL)) == move_index);
	assert((1ULL << move_square) == move_bit);

	for (uint32_t square = 0; bitscan_forward64(bb_moves, &square); bb_moves &= bb_moves - 1) {

		const uint64_t f1 = flip(square, player, opponent);
		const uint64_t f2 = flip_naive(square, player, opponent);

		assert(f1 == f2);

		const int32_t i1 = integrated_move_scoring(player, opponent, square);
		const int32_t i2 = integrated_move_scoring_naive(player, opponent, square);

		assert(i1 == i2);

		const uint64_t next_p = opponent ^ f1;
		const uint64_t next_o = player ^ (f1 | (1ULL << square));
		assert((next_p & next_o) == 0);

		uint64_t unique_p1 = 0, unique_p2 = 0, unique_o1 = 0, unique_o2 = 0;

		board_unique_naive(next_p, next_o, unique_p1, unique_o1);
		board_unique(next_p, next_o, unique_p2, unique_o2);

		assert((unique_p1 == unique_p2) && (unique_o1 == unique_o2));
	}

	const uint64_t f = flip(move_square, player, opponent);

	const uint64_t next_player = opponent ^ f;
	const uint64_t next_opponent = player ^ (f | move_bit);

	return uint64_t(score1 + 65 + n_empties) * test_random_playout(next_player, next_opponent, rnd);
}

void unittest_misc(const int num, const int seed) {

	std::mt19937_64 rnd(seed);
	uint64_t fingerprint = 0;
	std::cout << "start: unittest_misc" << std::endl;
	for (int i = 1; i <= num; ++i) {
		if (i % (num / 10) == 0 && i < num) {
			std::cout << "log: unittest_misc: " << i << " / " << num << std::endl;
		}
		fingerprint ^= test_random_playout((1ULL << E4) + (1ULL << D5), (1ULL << D4) + (1ULL << E5), rnd);//1手目をF5に打つ慣習があるので、playerとopponentはこの順番であるべき。
	}
	std::cout << "finish: unittest_misc, fingerprint = " << std::hex << fingerprint << std::dec << std::endl;

}