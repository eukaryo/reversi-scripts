#pragma once

#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <regex>
#include <thread>
#include <random>
#include <array>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include <fstream>
#include <functional>
#include <filesystem>
#include <numeric>
#include <utility>

#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>


std::string bitboard2obf(const uint64_t player, const uint64_t opponent);
void obf2bitboard(const std::string &obf, uint64_t &player, uint64_t &opponent);
void board_unique(const uint64_t original_player, const uint64_t original_opponent, uint64_t &player, uint64_t &opponent);
std::array<uint64_t, 2>board_unique(const uint64_t original_player, const uint64_t original_opponent);
std::array<uint64_t, 2>board_unique(const std::array<uint64_t, 2> original_bitboards);
std::array<uint64_t, 2>board_unique(const std::string &obf);
//uint64_t HorizontalOr(const __m256i x);
bool IsObfFormat(const std::string &s);

void init_hash_rank();
uint64_t get_hash_code(uint64_t player, uint64_t opponent);

inline uint32_t bitscan_forward64(const uint64_t x, uint32_t *dest) {

	//xが非ゼロなら、立っているビットのうち最下位のものの位置をdestに代入して、非ゼロの値を返す。
	//xがゼロなら、ゼロを返す。このときのdestの値は未定義である。

#ifdef _MSC_VER
	return _BitScanForward64(reinterpret_cast<unsigned long *>(dest), x);
#else
	return x ? *dest = __builtin_ctzll(x), 1 : 0;
#endif

}

enum {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	PASS, NOMOVE
};

constexpr int32_t SCORE_MIN = -64;
constexpr int32_t SCORE_MAX = 64;
constexpr int32_t MAX_MOVE = 34;

void init_eval_json(const std::string &);

int32_t EvaluatePosition0(const uint64_t player, const uint64_t opponent);

uint64_t flip(const uint64_t square, const uint64_t player, const uint64_t opponent);
uint64_t get_moves(const uint64_t P, const uint64_t O);
int32_t integrated_move_scoring(const uint64_t player, const uint64_t opponent, const int32_t pos);
int32_t integrated_move_scoring(const uint64_t player, const uint64_t opponent, const int32_t pos, const uint64_t bb_moves);

inline uint64_t pdep_intrinsics(uint64_t a, uint64_t mask) { return _pdep_u64(a, mask); }
inline uint64_t pext_intrinsics(uint64_t a, uint64_t mask) { return _pext_u64(a, mask); }

constexpr uint64_t initial_occupied = (1ULL << 27) + (1ULL << 28) + (1ULL << 35) + (1ULL << 36);
constexpr uint64_t initial_empty = ~initial_occupied;

void unittest_misc(const int num, const int seed);

int32_t EvaluatePosition0_naive(const uint64_t player, const uint64_t opponent);

void eval_set_pext_print();