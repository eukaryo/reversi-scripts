/**
 * @file eval.c
 *
 * Evaluation function.
 *
 * @date 1998 - 2017
 * @author Richard Delorme
 * @version 4.4
 */

 /*
  * @author Hiroki Takizawa
  * @date 2020
  */

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "Header.hpp"

#include "json.hpp"


enum {
	BLACK = 0,
	WHITE,
};

constexpr int32_t EVAL_N_WEIGHT = 226315;
constexpr int32_t EVAL_N_PLY = 61;
constexpr int32_t EVAL_N_FEATURE = 47;

typedef struct Board {
	uint64_t player, opponent;
} Board;
typedef struct Eval {
	int feature[EVAL_N_FEATURE];
	int player;
}Eval;



/** coordinate to feature conversion */
typedef struct CoordinateToFeature {
	int32_t n_feature;
	struct {
		int32_t i;
		int32_t x;
	} feature[16];
} CoordinateToFeature;

/** feature to coordinates conversion */
typedef struct FeatureToCoordinate {
	int32_t n_square;
	int32_t x[16];
} FeatureToCoordinate;

/** array to convert features into coordinates */
static const FeatureToCoordinate EVAL_F2X[] = {
	{ 9, {A1, B1, A2, B2, C1, A3, C2, B3, C3}},
	{ 9, {H1, G1, H2, G2, F1, H3, F2, G3, F3}},
	{ 9, {A8, A7, B8, B7, A6, C8, B6, C7, C6}},
	{ 9, {H8, H7, G8, G7, H6, F8, G6, F7, F6}},

	{10, {A5, A4, A3, A2, A1, B2, B1, C1, D1, E1}},
	{10, {H5, H4, H3, H2, H1, G2, G1, F1, E1, D1}},
	{10, {A4, A5, A6, A7, A8, B7, B8, C8, D8, E8}},
	{10, {H4, H5, H6, H7, H8, G7, G8, F8, E8, D8}},

	{10, {B2, A1, B1, C1, D1, E1, F1, G1, H1, G2}},
	{10, {B7, A8, B8, C8, D8, E8, F8, G8, H8, G7}},
	{10, {B2, A1, A2, A3, A4, A5, A6, A7, A8, B7}},
	{10, {G2, H1, H2, H3, H4, H5, H6, H7, H8, G7}},

	{10, {A1, C1, D1, C2, D2, E2, F2, E1, F1, H1}},
	{10, {A8, C8, D8, C7, D7, E7, F7, E8, F8, H8}},
	{10, {A1, A3, A4, B3, B4, B5, B6, A5, A6, A8}},
	{10, {H1, H3, H4, G3, G4, G5, G6, H5, H6, H8}},

	{ 8, {A2, B2, C2, D2, E2, F2, G2, H2}},
	{ 8, {A7, B7, C7, D7, E7, F7, G7, H7}},
	{ 8, {B1, B2, B3, B4, B5, B6, B7, B8}},
	{ 8, {G1, G2, G3, G4, G5, G6, G7, G8}},

	{ 8, {A3, B3, C3, D3, E3, F3, G3, H3}},
	{ 8, {A6, B6, C6, D6, E6, F6, G6, H6}},
	{ 8, {C1, C2, C3, C4, C5, C6, C7, C8}},
	{ 8, {F1, F2, F3, F4, F5, F6, F7, F8}},

	{ 8, {A4, B4, C4, D4, E4, F4, G4, H4}},
	{ 8, {A5, B5, C5, D5, E5, F5, G5, H5}},
	{ 8, {D1, D2, D3, D4, D5, D6, D7, D8}},
	{ 8, {E1, E2, E3, E4, E5, E6, E7, E8}},

	{ 8, {A1, B2, C3, D4, E5, F6, G7, H8}},
	{ 8, {A8, B7, C6, D5, E4, F3, G2, H1}},

	{ 7, {B1, C2, D3, E4, F5, G6, H7}},
	{ 7, {H2, G3, F4, E5, D6, C7, B8}},
	{ 7, {A2, B3, C4, D5, E6, F7, G8}},
	{ 7, {G1, F2, E3, D4, C5, B6, A7}},

	{ 6, {C1, D2, E3, F4, G5, H6}},
	{ 6, {A3, B4, C5, D6, E7, F8}},
	{ 6, {F1, E2, D3, C4, B5, A6}},
	{ 6, {H3, G4, F5, E6, D7, C8}},

	{ 5, {D1, E2, F3, G4, H5}},
	{ 5, {A4, B5, C6, D7, E8}},
	{ 5, {E1, D2, C3, B4, A5}},
	{ 5, {H4, G5, F6, E7, D8}},

	{ 4, {D1, C2, B3, A4}},
	{ 4, {A5, B6, C7, D8}},
	{ 4, {E1, F2, G3, H4}},
	{ 4, {H5, G6, F7, E8}},

	{ 0, {NOMOVE}},
	{ 0, {NOMOVE}}
};

/** array to convert coordinates into feature */
static const CoordinateToFeature EVAL_X2F[] = {
	{7, {{ 0,    6561}, { 4,     243}, { 8,    6561}, {10,    6561}, {12,   19683}, {14,   19683}, {28,    2187}}},  /* a1 */
	{5, {{ 0,    2187}, { 4,      27}, { 8,    2187}, {18,    2187}, {30,     729}}},                                /* b1 */
	{6, {{ 0,      81}, { 4,       9}, { 8,     729}, {12,    6561}, {22,    2187}, {34,     243}}},                 /* c1 */
	{7, {{ 4,       3}, { 5,       1}, { 8,     243}, {12,    2187}, {26,    2187}, {38,      81}, {42,      27}}},  /* d1 */
	{7, {{ 4,       1}, { 5,       3}, { 8,      81}, {12,       9}, {27,    2187}, {40,      81}, {44,      27}}},  /* e1 */
	{6, {{ 1,      81}, { 5,       9}, { 8,      27}, {12,       3}, {23,    2187}, {36,     243}}},                 /* f1 */
	{5, {{ 1,    2187}, { 5,      27}, { 8,       9}, {19,    2187}, {33,     729}}},                                /* g1 */
	{7, {{ 1,    6561}, { 5,     243}, { 8,       3}, {11,    6561}, {12,       1}, {15,   19683}, {29,       1}}},  /* h1 */
	{5, {{ 0,     729}, { 4,     729}, {10,    2187}, {16,    2187}, {32,     729}}},                                /* a2 */
	{7, {{ 0,     243}, { 4,      81}, { 8,   19683}, {10,   19683}, {16,     729}, {18,     729}, {28,     729}}},  /* b2 */
	{6, {{ 0,       9}, {12,     729}, {16,     243}, {22,     729}, {30,     243}, {42,       9}}},                 /* c2 */
	{5, {{12,     243}, {16,      81}, {26,     729}, {34,      81}, {40,      27}}},                                /* d2 */
	{5, {{12,      81}, {16,      27}, {27,     729}, {36,      81}, {38,      27}}},                                /* e2 */
	{6, {{ 1,       9}, {12,      27}, {16,       9}, {23,     729}, {33,     243}, {44,       9}}},                 /* f2 */
	{7, {{ 1,     243}, { 5,      81}, { 8,       1}, {11,   19683}, {16,       3}, {19,     729}, {29,       3}}},  /* g2 */
	{5, {{ 1,     729}, { 5,     729}, {11,    2187}, {16,       1}, {31,     729}}},                                /* h2 */
	{6, {{ 0,      27}, { 4,    2187}, {10,     729}, {14,    6561}, {20,    2187}, {35,     243}}},                 /* a3 */
	{6, {{ 0,       3}, {14,     729}, {18,     243}, {20,     729}, {32,     243}, {42,       3}}},                 /* b3 */
	{5, {{ 0,       1}, {20,     243}, {22,     243}, {28,     243}, {40,       9}}},                                /* c3 */
	{4, {{20,      81}, {26,     243}, {30,      81}, {36,      27}}},                                               /* d3 */
	{4, {{20,      27}, {27,     243}, {33,      81}, {34,      27}}},                                               /* e3 */
	{5, {{ 1,       1}, {20,       9}, {23,     243}, {29,       9}, {38,       9}}},                                /* f3 */
	{6, {{ 1,       3}, {15,     729}, {19,     243}, {20,       3}, {31,     243}, {44,       3}}},                 /* g3 */
	{6, {{ 1,      27}, { 5,    2187}, {11,     729}, {15,    6561}, {20,       1}, {37,     243}}},                 /* h3 */
	{7, {{ 4,    6561}, { 6,   19683}, {10,     243}, {14,    2187}, {24,    2187}, {39,      81}, {42,       1}}},  /* a4 */
	{5, {{14,     243}, {18,      81}, {24,     729}, {35,      81}, {40,       3}}},                                /* b4 */
	{4, {{22,      81}, {24,     243}, {32,      81}, {36,       9}}},                                               /* c4 */
	{4, {{24,      81}, {26,      81}, {28,      81}, {33,      27}}},                                               /* d4 */
	{4, {{24,      27}, {27,      81}, {29,      27}, {30,      27}}},                                               /* e4 */
	{4, {{23,      81}, {24,       9}, {31,      81}, {34,       9}}},                                               /* f4 */
	{5, {{15,     243}, {19,      81}, {24,       3}, {37,      81}, {38,       3}}},                                /* g4 */
	{7, {{ 5,    6561}, { 7,   19683}, {11,     243}, {15,    2187}, {24,       1}, {41,      81}, {44,       1}}},  /* h4 */
	{7, {{ 4,   19683}, { 6,    6561}, {10,      81}, {14,       9}, {25,    2187}, {40,       1}, {43,      27}}},  /* a5 */
	{5, {{14,      81}, {18,      27}, {25,     729}, {36,       3}, {39,      27}}},                                /* b5 */
	{4, {{22,      27}, {25,     243}, {33,       9}, {35,      27}}},                                               /* c5 */
	{4, {{25,      81}, {26,      27}, {29,      81}, {32,      27}}},                                               /* d5 */
	{4, {{25,      27}, {27,      27}, {28,      27}, {31,      27}}},                                               /* e5 */
	{4, {{23,      27}, {25,       9}, {30,       9}, {37,      27}}},                                               /* f5 */
	{5, {{15,      81}, {19,      27}, {25,       3}, {34,       3}, {41,      27}}},                                /* g5 */
	{7, {{ 5,   19683}, { 7,    6561}, {11,      81}, {15,       9}, {25,       1}, {38,       1}, {45,      27}}},  /* h5 */
	{6, {{ 2,      81}, { 6,    2187}, {10,      27}, {14,       3}, {21,    2187}, {36,       1}}},                 /* a6 */
	{6, {{ 2,       9}, {14,      27}, {18,       9}, {21,     729}, {33,       3}, {43,       9}}},                 /* b6 */
	{5, {{ 2,       1}, {21,     243}, {22,       9}, {29,     243}, {39,       9}}},                                /* c6 */
	{4, {{21,      81}, {26,       9}, {31,       9}, {35,       9}}},                                               /* d6 */
	{4, {{21,      27}, {27,       9}, {32,       9}, {37,       9}}},                                               /* e6 */
	{5, {{ 3,       1}, {21,       9}, {23,       9}, {28,       9}, {41,       9}}},                                /* f6 */
	{6, {{ 3,       9}, {15,      27}, {19,       9}, {21,       3}, {30,       3}, {45,       9}}},                 /* g6 */
	{6, {{ 3,      81}, { 7,    2187}, {11,      27}, {15,       3}, {21,       1}, {34,       1}}},                 /* h6 */
	{5, {{ 2,    2187}, { 6,     729}, {10,       9}, {17,    2187}, {33,       1}}},                                /* a7 */
	{7, {{ 2,     243}, { 6,      81}, { 9,   19683}, {10,       1}, {17,     729}, {18,       3}, {29,     729}}},  /* b7 */
	{6, {{ 2,       3}, {13,     729}, {17,     243}, {22,       3}, {31,       3}, {43,       3}}},                 /* c7 */
	{5, {{13,     243}, {17,      81}, {26,       3}, {37,       3}, {39,       3}}},                                /* d7 */
	{5, {{13,      81}, {17,      27}, {27,       3}, {35,       3}, {41,       3}}},                                /* e7 */
	{6, {{ 3,       3}, {13,      27}, {17,       9}, {23,       3}, {32,       3}, {45,       3}}},                 /* f7 */
	{7, {{ 3,     243}, { 7,      81}, { 9,       1}, {11,       1}, {17,       3}, {19,       3}, {28,       3}}},  /* g7 */
	{5, {{ 3,    2187}, { 7,     729}, {11,       9}, {17,       1}, {30,       1}}},                                /* h7 */
	{7, {{ 2,    6561}, { 6,     243}, { 9,    6561}, {10,       3}, {13,   19683}, {14,       1}, {29,    2187}}},  /* a8 */
	{5, {{ 2,     729}, { 6,      27}, { 9,    2187}, {18,       1}, {31,       1}}},                                /* b8 */
	{6, {{ 2,      27}, { 6,       9}, { 9,     729}, {13,    6561}, {22,       1}, {37,       1}}},                 /* c8 */
	{7, {{ 6,       3}, { 7,       1}, { 9,     243}, {13,    2187}, {26,       1}, {41,       1}, {43,       1}}},  /* d8 */
	{7, {{ 6,       1}, { 7,       3}, { 9,      81}, {13,       9}, {27,       1}, {39,       1}, {45,       1}}},  /* e8 */
	{6, {{ 3,      27}, { 7,       9}, { 9,      27}, {13,       3}, {23,       1}, {35,       1}}},                 /* f8 */
	{5, {{ 3,     729}, { 7,      27}, { 9,       9}, {19,       1}, {32,       1}}},                                /* g8 */
	{7, {{ 3,    6561}, { 7,     243}, { 9,       3}, {11,       3}, {13,       1}, {15,       1}, {28,       1}}},  /* h8 */
	{0, {{ 0, 0}}} // <- PASS
};

/** feature size */
static const int32_t EVAL_SIZE[] = { 19683, 59049, 59049, 59049, 6561, 6561, 6561, 6561, 2187, 729, 243, 81, 1 };

/** packed feature size */
static const int32_t EVAL_PACKED_SIZE[] = { 10206, 29889, 29646, 29646, 3321, 3321, 3321, 3321, 1134, 378, 135, 45, 1 };

/** feature offset */
alignas(64) static const int32_t EVAL_OFFSET[] = {
		 0,      0,      0,      0,
	 19683,  19683,  19683,  19683,
	 78732,  78732,  78732,  78732,
	137781,	137781, 137781, 137781,
	196830,	196830, 196830, 196830,
	203391,	203391, 203391, 203391,
	209952,	209952, 209952, 209952,
	216513,	216513,
	223074,	223074,	223074,	223074,
	225261,	225261,	225261,	225261,
	225990,	225990, 225990,	225990,
	226233,	226233, 226233, 226233,
	226314,
	0
};

/** feature symetry packing */
static int32_t EVAL_C10[2][59049];
static int32_t EVAL_S10[2][59049];
static int32_t EVAL_C9[2][19683];
static int32_t EVAL_S8[2][6561];
static int32_t EVAL_S7[2][2187];
static int32_t EVAL_S6[2][729];
static int32_t EVAL_S5[2][243];
static int32_t EVAL_S4[2][81];


/** eval weight load status */
static int32_t EVAL_LOADED = 0;

/** eval weights */
//short ***EVAL_WEIGHT;
std::array<std::array<std::array<int16_t, EVAL_N_WEIGHT>, EVAL_N_PLY>, 2>EVAL_WEIGHT;


/** evaluation function error coefficient parameters */
static double EVAL_A, EVAL_B, EVAL_C, EVAL_a, EVAL_b, EVAL_c;



alignas(32) uint64_t SAG_CONSTANT[EVAL_N_FEATURE + 1][6] = {};
alignas(32) uint8_t SAG_LENGTH[EVAL_N_FEATURE + 1] = {};

alignas(32) uint16_t BIT_REVERSE_TABLE[11][1024] = {};

static uint16_t reverse_bit(uint16_t number, uint16_t length) {

	assert(length <= 10);
	assert(number < (1 << length));

	uint16_t answer = 0;
	for (uint16_t i = 0; i < length; ++i) {
		if (number & (1 << i)) {
			answer |= (1 << (length - i - 1));
		}
	}

	assert(answer < (1 << length));
	return answer;
}

static uint64_t compress(uint64_t x, uint64_t mask) {
	return pext_intrinsics(x, mask);
}

static uint64_t sag(uint64_t x, uint64_t mask) {
	return (compress(x, mask) << (_mm_popcnt_u64(~mask)))
		| compress(x, ~mask);
}

static void init_eval_tables_pext() {

	for (uint16_t length = 4; length <= 10; ++length) {
		for (uint16_t number = 0; number < (1 << length); ++number) {
			BIT_REVERSE_TABLE[length][number] = reverse_bit(number, length);
		}
	}

	for (int n = 0; n < EVAL_N_FEATURE; ++n) {

		int x[64] = {};
		for (int i = 0; i < 64; ++i)x[i] = -1;

		for (int i = 0; i < EVAL_F2X[n].n_square; i++) {
			//x[EVAL_F2X[n].x[i]] = EVAL_F2X[n].n_square - 1 - i;
			x[EVAL_F2X[n].x[i]] = i;
		}

		for (int i = 0, count = EVAL_F2X[n].n_square; i < 64; ++i) {
			if (x[i] == -1) {
				x[i] = count++;
			}
		}

		//この時点でxは0～63の順列である。
		for (int i = 0; i < 64; ++i)
			assert(0 <= x[i] && x[i] < 64);
		for (int i = 0; i < 64; ++i)for (int j = i + 1; j < 64; ++j)
			assert(x[i] != x[j]);

		int x_stable[64] = {};
		int rep = 0;
		for (int count = 0; count < 64; ++rep) {
			for (int i = 0; i < 64; ++i) {
				if (x[i] == count) {
					x_stable[i] = rep;
					count++;
				}
			}
		}

		//冪等性を保つためにここでゼロ埋めする。
		for (int i = 0; i < 6; ++i)SAG_CONSTANT[n][i] = 0;

		for (int i = 0; i < 64; ++i) {
			for (uint64_t b = x_stable[i], j = 0; b; ++j, b >>= 1) {
				if (b & 1ULL) {
					SAG_CONSTANT[n][j] |= 1ULL << i;
				}
			}
		}

		SAG_CONSTANT[n][1] = sag(SAG_CONSTANT[n][1], SAG_CONSTANT[n][0]);
		SAG_CONSTANT[n][2] = sag(sag(SAG_CONSTANT[n][2], SAG_CONSTANT[n][0]), SAG_CONSTANT[n][1]);
		SAG_CONSTANT[n][3] = sag(sag(sag(SAG_CONSTANT[n][3], SAG_CONSTANT[n][0]), SAG_CONSTANT[n][1]), SAG_CONSTANT[n][2]);
		SAG_CONSTANT[n][4] = sag(sag(sag(sag(SAG_CONSTANT[n][4], SAG_CONSTANT[n][0]), SAG_CONSTANT[n][1]), SAG_CONSTANT[n][2]), SAG_CONSTANT[n][3]);
		SAG_CONSTANT[n][5] = sag(sag(sag(sag(sag(SAG_CONSTANT[n][5], SAG_CONSTANT[n][0]), SAG_CONSTANT[n][1]), SAG_CONSTANT[n][2]), SAG_CONSTANT[n][3]), SAG_CONSTANT[n][4]);

		for (int i = 0; i < 6; ++i) {
			if (SAG_CONSTANT[n][i] == uint64_t(0xFFFF'FFFF'FFFF'FFFFULL)) {
				assert(i == 5 || SAG_CONSTANT[n][i + 1] == 0);
				SAG_CONSTANT[n][i] = 0;
			}
		}

		for (SAG_LENGTH[n] = 0; SAG_CONSTANT[n][SAG_LENGTH[n]] != 0; ++SAG_LENGTH[n]);
	}
}

static void util_info(const std::string &message) {
	std::cerr << "info: " << message << std::endl;
}
static void util_fatal_error(const std::string &error_message) {

	std::cerr << "fatal error: " << error_message << std::endl;
	std::cout << "fatal error: " << error_message << std::endl;
	std::exit(EXIT_FAILURE);
}
static int board_get_square_color(const Board &board, const int x)
{
	return 2 - 2 * ((board.player >> x) & 1) - ((board.opponent >> x) & 1);
}
/**
 * @brief Opponent feature.
 *
 * Compute a feature from the opponent point of view.
 * @param l feature.
 * @param d feature size.
 * @return opponent feature.
 */
static int opponent_feature(int32_t l, int32_t d)
{
	static const int32_t o[] = { 1, 0, 2 };
	int32_t f = o[l % 3];

	if (d > 1) f += opponent_feature(l / 3, d - 1) * 3;

	return f;
}

static void create_unpacking_tables() {

	std::vector<int32_t>T(59049, 0);

	for (int32_t l = 0, n = 0; l < 6561; l++) { /* 8 squares : 6561 -> 3321 */
		int32_t k = ((l / 2187) % 3) + ((l / 729) % 3) * 3 + ((l / 243) % 3) * 9 +
			((l / 81) % 3) * 27 + ((l / 27) % 3) * 81 + ((l / 9) % 3) * 243 +
			((l / 3) % 3) * 729 + (l % 3) * 2187;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S8[0][l] = T[l];
		EVAL_S8[1][opponent_feature(l, 8)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 2187; l++) { /* 7 squares : 2187 -> 1134 */
		int32_t k = ((l / 729) % 3) + ((l / 243) % 3) * 3 + ((l / 81) % 3) * 9 +
			((l / 27) % 3) * 27 + ((l / 9) % 3) * 81 + ((l / 3) % 3) * 243 +
			(l % 3) * 729;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S7[0][l] = T[l];
		EVAL_S7[1][opponent_feature(l, 7)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 729; l++) { /* 6 squares : 729 -> 378 */
		int32_t k = ((l / 243) % 3) + ((l / 81) % 3) * 3 + ((l / 27) % 3) * 9 +
			((l / 9) % 3) * 27 + ((l / 3) % 3) * 81 + (l % 3) * 243;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S6[0][l] = T[l];
		EVAL_S6[1][opponent_feature(l, 6)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 243; l++) { /* 5 squares : 243 -> 135 */
		int32_t k = ((l / 81) % 3) + ((l / 27) % 3) * 3 + ((l / 9) % 3) * 9 +
			((l / 3) % 3) * 27 + (l % 3) * 81;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S5[0][l] = T[l];
		EVAL_S5[1][opponent_feature(l, 5)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 81; l++) { /* 4 squares : 81 -> 45 */
		int32_t k = ((l / 27) % 3) + ((l / 9) % 3) * 3 + ((l / 3) % 3) * 9 + (l % 3) * 27;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S4[0][l] = T[l];
		EVAL_S4[1][opponent_feature(l, 4)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 19683; l++) { /* 9 corner squares : 19683 -> 10206 */
		int32_t k = ((l / 6561) % 3) * 6561 + ((l / 729) % 3) * 2187 +
			((l / 2187) % 3) * 729 + ((l / 243) % 3) * 243 + ((l / 27) % 3) * 81 +
			((l / 81) % 3) * 27 + ((l / 3) % 3) * 9 + ((l / 9) % 3) * 3 + (l % 3);
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_C9[0][l] = T[l];
		EVAL_C9[1][opponent_feature(l, 9)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 59049; l++) { /* 10 squares (edge +X ) : 59049 -> 29646 */
		int32_t k = ((l / 19683) % 3) + ((l / 6561) % 3) * 3 + ((l / 2187) % 3) * 9 +
			((l / 729) % 3) * 27 + ((l / 243) % 3) * 81 + ((l / 81) % 3) * 243 +
			((l / 27) % 3) * 729 + ((l / 9) % 3) * 2187 + ((l / 3) % 3) * 6561 +
			(l % 3) * 19683;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_S10[0][l] = T[l];
		EVAL_S10[1][opponent_feature(l, 10)] = T[l];
	}
	for (int32_t l = 0, n = 0; l < 59049; l++) { /* 10 squares (angle + X) : 59049 -> 29889 */
		int32_t k = ((l / 19683) % 3) + ((l / 6561) % 3) * 3 + ((l / 2187) % 3) * 9 +
			((l / 729) % 3) * 27 + ((l / 243) % 3) * 243 + ((l / 81) % 3) * 81 +
			((l / 27) % 3) * 729 + ((l / 9) % 3) * 2187 + ((l / 3) % 3) * 6561 +
			(l % 3) * 19683;
		if (k < l) T[l] = T[k];
		else T[l] = n++;
		EVAL_C10[0][l] = T[l];
		EVAL_C10[1][opponent_feature(l, 10)] = T[l];
	}
}

static void unpack_data(const std::vector<int16_t> &w, const int32_t ply) {
	int32_t i = 0, j = 0, offset = 0;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_C9[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_C9[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_C10[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_C10[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S10[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S10[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S10[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S10[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S8[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S8[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S7[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S7[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S6[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S6[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S5[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S5[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	for (int32_t k = 0; k < EVAL_SIZE[i]; k++, j++) {
		EVAL_WEIGHT[0][ply][j] = w[EVAL_S4[0][k] + offset];
		EVAL_WEIGHT[1][ply][j] = w[EVAL_S4[1][k] + offset];
	}
	offset += EVAL_PACKED_SIZE[i];
	i++;
	EVAL_WEIGHT[0][ply][j] = w[offset];
	EVAL_WEIGHT[1][ply][j] = w[offset];
}

void init_eval_json(const std::string &file)
{
	init_eval_tables_pext();

	if (EVAL_LOADED++) return;

	// create unpacking tables
	create_unpacking_tables();

	nlohmann::json j;

	try {
		std::ifstream i(file);
		i >> j;
	}
	catch (...) {
		util_fatal_error("init_eval_json: Cannot open the eval data.");
	}

	uint32_t version, release, build;

	try {
		const int32_t n_w = 114364;

		std::vector<int16_t>w(n_w, 0);

		version = std::stoi(j["version"].get<std::string>());
		release = std::stoi(j["release"].get<std::string>());
		build = std::stoi(j["build"].get<std::string>());

		// Weights : read & unpacked them
		for (int32_t ply = 0; ply < EVAL_N_PLY; ply++) {
			std::string p = std::to_string(ply);
			for (int32_t i = 0; i < n_w; ++i)w[i] = j["weight"][p][i].get<int16_t>();
			unpack_data(w, ply);
		}
	}
	catch (...) {
		util_fatal_error("init_eval_json: Cannot read the eval data.");
	}

	/*if (version == 3 && release == 2 && build == 5)*/ {
		EVAL_A = -0.10026799, EVAL_B = 0.31027733, EVAL_C = -0.57772603;
		EVAL_a = 0.07585621, EVAL_b = 1.16492647, EVAL_c = 5.4171698;
	}

	util_info(std::string("<init_eval_json: Evaluation function weights version ") +
		std::to_string(version) + std::string(".") +
		std::to_string(release) + std::string(".") +
		std::to_string(build) + std::string(" loaded>"));
}

/**
 * @brief Set up evaluation features from a board.
 *
 * @param eval  Evaluation function.
 * @param board Board to setup features from.
 */
static void eval_set(Eval *eval, const Board &board)
{
	int i, j, c;

	for (i = 0; i < EVAL_N_FEATURE; ++i) {
		eval->feature[i] = 0;
		for (j = 0; j < EVAL_F2X[i].n_square; j++) {
			c = board_get_square_color(board, EVAL_F2X[i].x[j]);
			eval->feature[i] = eval->feature[i] * 3 + c;
		}
		eval->feature[i] += EVAL_OFFSET[i];
	}
	eval->player = 0;
}

/**
 * @brief Compute the error-type of the evaluation function according to the
 * depths.
 *
 * A statistical study showed thet the accuracy of the alphabeta
 * mostly depends on the depth & the ply of the game. This function is useful
 * to the probcut algorithm. Using a function instead of a table of data makes
 * easier to inter- or extrapolate new values.
 *
 * @param n_empty Number of empty squares on the board.
 * @param depth Depth used in alphabeta.
 * @param probcut_depth A shallow depth used in probcut algorithm.
 */
//double eval_sigma(const int n_empty, const int depth, const int probcut_depth)
//{
//	double sigma;
//
//	sigma = EVAL_A * n_empty + EVAL_B * depth + EVAL_C * probcut_depth;
//	sigma = EVAL_a * sigma * sigma + EVAL_b * sigma + EVAL_c;
//
//	return sigma;
//}

int32_t EvaluatePosition0_naive(const uint64_t player, const uint64_t opponent)
{
	Board b;
	b.player = player;
	b.opponent = opponent;

	Eval e;
	eval_set(&e, b);

	const int empties = 64 - _mm_popcnt_u64(player | opponent);

	const int16_t * const w = EVAL_WEIGHT[0][60 - empties].data();
	const int32_t * const f = e.feature;


	int32_t score = w[f[0]] + w[f[1]] + w[f[2]] + w[f[3]]
		+ w[f[4]] + w[f[5]] + w[f[6]] + w[f[7]]
		+ w[f[8]] + w[f[9]] + w[f[10]] + w[f[11]]
		+ w[f[12]] + w[f[13]] + w[f[14]] + w[f[15]]
		+ w[f[16]] + w[f[17]] + w[f[18]] + w[f[19]]
		+ w[f[20]] + w[f[21]] + w[f[22]] + w[f[23]]
		+ w[f[24]] + w[f[25]]
		+ w[f[26]] + w[f[27]] + w[f[28]] + w[f[29]]
		+ w[f[30]] + w[f[31]] + w[f[32]] + w[f[33]]
		+ w[f[34]] + w[f[35]] + w[f[36]] + w[f[37]]
		+ w[f[38]] + w[f[39]] + w[f[40]] + w[f[41]]
		+ w[f[42]] + w[f[43]] + w[f[44]] + w[f[45]]
		+ w[f[46]];

	if (score > 0) score += 64;	else score -= 64;
	score /= 128;

	if (score <= SCORE_MIN) score = SCORE_MIN + 1;
	else if (score >= SCORE_MAX) score = SCORE_MAX - 1;

	return score;
}



static void ternarize_avx2_full(const uint16_t u8l8[16], int answer[8], const int eval_offset[8]) {

	assert((u8l8[0] & u8l8[4]) == 0);
	assert((u8l8[1] & u8l8[5]) == 0);
	assert((u8l8[2] & u8l8[6]) == 0);
	assert((u8l8[3] & u8l8[7]) == 0);
	assert((u8l8[8] & u8l8[12]) == 0);
	assert((u8l8[9] & u8l8[13]) == 0);
	assert((u8l8[10] & u8l8[14]) == 0);
	assert((u8l8[11] & u8l8[15]) == 0);

	const __m256i ternary_binary_table = _mm256_set_epi8(40, 39, 37, 36, 31, 30, 28, 27, 13, 12, 10, 9, 4, 3, 1, 0, 40, 39, 37, 36, 31, 30, 28, 27, 13, 12, 10, 9, 4, 3, 1, 0);
	const __m256i bitmask0F = _mm256_set1_epi8(0x0F);

	const __m256i lu_lo = _mm256_loadu_si256((__m256i*)u8l8);
	const __m256i lu_hi = _mm256_srli_epi64(lu_lo, 4);

	const __m256i u_4bits = _mm256_and_si256(_mm256_unpacklo_epi8(lu_lo, lu_hi), bitmask0F);
	const __m256i l_4bits = _mm256_and_si256(_mm256_unpackhi_epi8(lu_lo, lu_hi), bitmask0F);

	const __m256i ternarized_u_4bits = _mm256_shuffle_epi8(ternary_binary_table, u_4bits);
	const __m256i ternarized_l_4bits = _mm256_shuffle_epi8(ternary_binary_table, l_4bits);
	const __m256i answer_base_3p4_in_i8s = _mm256_add_epi8(_mm256_add_epi8(ternarized_u_4bits, ternarized_u_4bits), ternarized_l_4bits);

	const __m256i tmp_mask8_lo = _mm256_set_epi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);
	const __m256i tmp_shuf8_hi = _mm256_set_epi8(0xFF, 15, 0xFF, 13, 0xFF, 11, 0xFF, 9, 0xFF, 7, 0xFF, 5, 0xFF, 3, 0xFF, 1, 0xFF, 15, 0xFF, 13, 0xFF, 11, 0xFF, 9, 0xFF, 7, 0xFF, 5, 0xFF, 3, 0xFF, 1);
	const __m256i i16_81s = _mm256_set1_epi16(81);
	const __m256i answer_tmp8_lo = _mm256_and_si256(answer_base_3p4_in_i8s, tmp_mask8_lo);
	const __m256i answer_tmp8_hi = _mm256_shuffle_epi8(answer_base_3p4_in_i8s, tmp_shuf8_hi);
	const __m256i answer_base3p8_in_i16s = _mm256_add_epi16(answer_tmp8_lo, _mm256_mullo_epi16(answer_tmp8_hi, i16_81s));

	const __m256i tmp_base16 = _mm256_set_epi16(6561, 1, 6561, 1, 6561, 1, 6561, 1, 6561, 1, 6561, 1, 6561, 1, 6561, 1);
	const __m256i res1 = _mm256_hadd_epi16(_mm256_setzero_si256(), _mm256_mullo_epi16(tmp_base16, answer_base3p8_in_i16s));
	const __m256i res2 = _mm256_unpackhi_epi16(res1, _mm256_setzero_si256());
	const __m256i res3 = _mm256_add_epi32(res2, _mm256_loadu_si256((__m256i*)eval_offset));

	_mm256_storeu_si256((__m256i*)answer, res3);
}

static void eval_set_pext(int eval_feature[EVAL_N_FEATURE + 1], const uint64_t player, const uint64_t opponent) {
	const uint64_t empty = ~(player | opponent);
	alignas(32) uint16_t u8l8[16] = {};

	for (int i = 0; i < 6; ++i) {

		for (int j = 0; j < 2; ++j) {
			for (int k = 0; k < 4; ++k) {

				const int index = i * 8 + j * 4 + k;

				uint64_t e = empty;
				uint64_t o = opponent;

				for (int n = 0; n < SAG_LENGTH[index]; ++n) {
					e = sag(e, SAG_CONSTANT[index][n]);
					o = sag(o, SAG_CONSTANT[index][n]);
				}

				const uint64_t mask_bitlen = (1ULL << EVAL_F2X[index].n_square) - 1ULL;
				u8l8[j * 8 + k + 0] = BIT_REVERSE_TABLE[EVAL_F2X[index].n_square][e & mask_bitlen];
				u8l8[j * 8 + k + 4] = BIT_REVERSE_TABLE[EVAL_F2X[index].n_square][o & mask_bitlen];
			}
		}

		ternarize_avx2_full(u8l8, &eval_feature[i * 8], &EVAL_OFFSET[i * 8]);
	}
}

void eval_set_pext_unrolled(int eval_feature[EVAL_N_FEATURE + 1], const uint64_t player, const uint64_t opponent);

int32_t EvaluatePosition0(const uint64_t player, const uint64_t opponent)
{
	alignas(32) static int32_t f[EVAL_N_FEATURE + 1];

	//eval_set_pext(f, player, opponent);
	eval_set_pext_unrolled(f, player, opponent);

	const int empties = 64 - _mm_popcnt_u64(player | opponent);

	const int16_t * const w = EVAL_WEIGHT[0][60 - empties].data();

	int32_t score = w[f[0]] + w[f[1]] + w[f[2]] + w[f[3]]
		+ w[f[4]] + w[f[5]] + w[f[6]] + w[f[7]]
		+ w[f[8]] + w[f[9]] + w[f[10]] + w[f[11]]
		+ w[f[12]] + w[f[13]] + w[f[14]] + w[f[15]]
		+ w[f[16]] + w[f[17]] + w[f[18]] + w[f[19]]
		+ w[f[20]] + w[f[21]] + w[f[22]] + w[f[23]]
		+ w[f[24]] + w[f[25]]
		+ w[f[26]] + w[f[27]] + w[f[28]] + w[f[29]]
		+ w[f[30]] + w[f[31]] + w[f[32]] + w[f[33]]
		+ w[f[34]] + w[f[35]] + w[f[36]] + w[f[37]]
		+ w[f[38]] + w[f[39]] + w[f[40]] + w[f[41]]
		+ w[f[42]] + w[f[43]] + w[f[44]] + w[f[45]]
		+ w[f[46]];

	if (score > 0) score += 64;	else score -= 64;
	score /= 128;

	if (score <= SCORE_MIN) score = SCORE_MIN + 1;
	else if (score >= SCORE_MAX) score = SCORE_MAX - 1;

	return score;
}

void eval_set_pext_print()
{
	//eval_set_pext_unrolled関数のソースコードを作って表示する。

	init_eval_tables_pext();

	for (int n = 0; n < EVAL_N_FEATURE; ++n) {
		for (int i = 0; i < SAG_LENGTH[n]; ++i) {
			const std::string sagconst = std::string("SAG_CONSTANT_") + std::to_string(n) + std::string("_") + std::to_string(i);
			std::cout << "constexpr uint64_t " << sagconst << " = " << SAG_CONSTANT[n][i] << "ULL;" << std::endl;
		}
	}

	std::cout << " void eval_set_pext_unrolled(int eval_feature[EVAL_N_FEATURE + 1], const uint64_t player, const uint64_t opponent) {" << std::endl;
	std::cout << "const uint64_t empty = ~(player | opponent);" << std::endl;
	std::cout << "alignas(32) uint16_t u8l8[16] = {};" << std::endl;

	for (int i = 0; i < 6; ++i) {
		std::cout << "{ // for (int i = 0; i < 6; ++i) {" << std::endl;

		for (int j = 0; j < 2; ++j) {
			std::cout << "{ // for (int j = 0; j < 2; ++j) {" << std::endl;
			for (int k = 0; k < 4; ++k) {

				std::cout << "{ // for (int k = 0; k < 4; ++k) {" << std::endl;
				const int index = i * 8 + j * 4 + k;
				std::cout << "// index = " << index << std::endl;

				if (EVAL_F2X[index].n_square == 0) {
					std::cout << "u8l8[" << j * 8 + k + 0 << "] = 0;" << std::endl;
					std::cout << "u8l8[" << j * 8 + k + 4 << "] = 0;" << std::endl;
				}
				else if (SAG_LENGTH[index] == 0) {
					std::cout << "constexpr uint64_t mask_bitlen = " << (1ULL << EVAL_F2X[index].n_square) - 1ULL << ";" << std::endl;
					std::cout << "u8l8[" << j * 8 + k + 0 << "] = BIT_REVERSE_TABLE[" << EVAL_F2X[index].n_square << "][empty & mask_bitlen];" << std::endl;
					std::cout << "u8l8[" << j * 8 + k + 4 << "] = BIT_REVERSE_TABLE[" << EVAL_F2X[index].n_square << "][opponent & mask_bitlen];" << std::endl;
				}
				else {

					std::cout << "uint64_t e = empty;" << std::endl;
					std::cout << "uint64_t o = opponent;" << std::endl;

					bool no_mask_flag = false;

					for (int n = 0; n < SAG_LENGTH[index]; ++n) {
						//std::cout << "{ // for (int n = 0; n < SAG_LENGTH[index]; ++n) {" << std::endl;

						const std::string sagconst = std::string("SAG_CONSTANT_") + std::to_string(index) + std::string("_") + std::to_string(n);

						if (n + 1 == SAG_LENGTH[index] && _mm_popcnt_u64(~SAG_CONSTANT[index][n]) == EVAL_F2X[index].n_square) {
							std::cout << "e = _pext_u64(e, ~" << sagconst << ");" << std::endl;
							std::cout << "o = _pext_u64(o, ~" << sagconst << ");" << std::endl;
							no_mask_flag = true;
						}
						else {
							std::cout << "e = (_pext_u64(e, " << sagconst << ") << " << _mm_popcnt_u64(~SAG_CONSTANT[index][n]) << ") | _pext_u64(e, ~" << sagconst << ");" << std::endl;
							std::cout << "o = (_pext_u64(o, " << sagconst << ") << " << _mm_popcnt_u64(~SAG_CONSTANT[index][n]) << ") | _pext_u64(o, ~" << sagconst << ");" << std::endl;
						}

						//std::cout << "}" << std::endl;
					}
					if (no_mask_flag) {
						std::cout << "u8l8[" << j * 8 + k + 0 << "] = BIT_REVERSE_TABLE[" << EVAL_F2X[index].n_square << "][e];" << std::endl;
						std::cout << "u8l8[" << j * 8 + k + 4 << "] = BIT_REVERSE_TABLE[" << EVAL_F2X[index].n_square << "][o];" << std::endl;
					}
					else {
						uint64_t mask_bitlen = (1ULL << EVAL_F2X[index].n_square) - 1ULL;
						std::cout << "u8l8[" << j * 8 + k + 0 << "] = BIT_REVERSE_TABLE[" << EVAL_F2X[index].n_square << "][e & " << mask_bitlen << "];" << std::endl;
						std::cout << "u8l8[" << j * 8 + k + 4 << "] = BIT_REVERSE_TABLE[" << EVAL_F2X[index].n_square << "][o & " << mask_bitlen << "];" << std::endl;
					}
				}
				std::cout << "}" << std::endl;
			}
			std::cout << "}" << std::endl;
		}

		std::cout << "ternarize_avx2_full(u8l8, &eval_feature[" << i * 8 << "], &EVAL_OFFSET[" << i * 8 << "]);" << std::endl;
		std::cout << "}" << std::endl;
	}


	std::cout << "}" << std::endl;
}




constexpr uint64_t SAG_CONSTANT_0_0 = 18446744073709156604ULL;
constexpr uint64_t SAG_CONSTANT_0_1 = 18446744073709289328ULL;
constexpr uint64_t SAG_CONSTANT_1_0 = 10518624ULL;
constexpr uint64_t SAG_CONSTANT_1_1 = 10376293541461358559ULL;
constexpr uint64_t SAG_CONSTANT_1_2 = 4611686018427322342ULL;
constexpr uint64_t SAG_CONSTANT_2_0 = 432632536762417152ULL;
constexpr uint64_t SAG_CONSTANT_2_1 = 10367144505206898687ULL;
constexpr uint64_t SAG_CONSTANT_2_2 = 4609434218613702603ULL;
constexpr uint64_t SAG_CONSTANT_3_0 = 6953733746520489984ULL;
constexpr uint64_t SAG_CONSTANT_3_1 = 6050550915000172543ULL;
constexpr uint64_t SAG_CONSTANT_3_2 = 4539628424389459915ULL;
constexpr uint64_t SAG_CONSTANT_4_0 = 18446744069414518270ULL;
constexpr uint64_t SAG_CONSTANT_4_1 = 2052ULL;
constexpr uint64_t SAG_CONSTANT_4_2 = 4611686018418999291ULL;
constexpr uint64_t SAG_CONSTANT_5_0 = 18446743523945332567ULL;
constexpr uint64_t SAG_CONSTANT_5_1 = 131602ULL;
constexpr uint64_t SAG_CONSTANT_5_2 = 5764607523034235014ULL;
constexpr uint64_t SAG_CONSTANT_5_3 = 144115188042301437ULL;
constexpr uint64_t SAG_CONSTANT_6_0 = 2162290771091259392ULL;
constexpr uint64_t SAG_CONSTANT_6_1 = 540149376484376575ULL;
constexpr uint64_t SAG_CONSTANT_7_0 = 6304898188927041535ULL;
constexpr uint64_t SAG_CONSTANT_7_1 = 4611686018427387936ULL;
constexpr uint64_t SAG_CONSTANT_7_2 = 2161727821137838032ULL;
constexpr uint64_t SAG_CONSTANT_8_0 = 16639ULL;
constexpr uint64_t SAG_CONSTANT_8_1 = 36028797018963965ULL;
constexpr uint64_t SAG_CONSTANT_9_0 = 18014398509481984ULL;
constexpr uint64_t SAG_CONSTANT_9_1 = 35465847065542655ULL;
constexpr uint64_t SAG_CONSTANT_10_0 = 18446181123756129791ULL;
constexpr uint64_t SAG_CONSTANT_10_1 = 18374121322071325690ULL;
constexpr uint64_t SAG_CONSTANT_11_0 = 18428729675200053247ULL;
constexpr uint64_t SAG_CONSTANT_11_1 = 9187060661035662846ULL;
constexpr uint64_t SAG_CONSTANT_12_0 = 176ULL;
constexpr uint64_t SAG_CONSTANT_12_1 = 2305843009213692018ULL;
constexpr uint64_t SAG_CONSTANT_13_0 = 12699025049277956096ULL;
constexpr uint64_t SAG_CONSTANT_13_1 = 85568392920039423ULL;
constexpr uint64_t SAG_CONSTANT_14_0 = 18374685375848185854ULL;
constexpr uint64_t SAG_CONSTANT_14_1 = 18446739641032769528ULL;
constexpr uint64_t SAG_CONSTANT_15_0 = 9223230747454734207ULL;
constexpr uint64_t SAG_CONSTANT_15_1 = 18446460382395498488ULL;
constexpr uint64_t SAG_CONSTANT_16_0 = 18446744073709486335ULL;
constexpr uint64_t SAG_CONSTANT_17_0 = 18374967954648334335ULL;
constexpr uint64_t SAG_CONSTANT_18_0 = 18302063728033398269ULL;
constexpr uint64_t SAG_CONSTANT_19_0 = 13816973012072644543ULL;
constexpr uint64_t SAG_CONSTANT_20_0 = 18446744073692839935ULL;
constexpr uint64_t SAG_CONSTANT_21_0 = 18446463698244468735ULL;
constexpr uint64_t SAG_CONSTANT_22_0 = 18157383382357244923ULL;
constexpr uint64_t SAG_CONSTANT_23_0 = 16131858542891098079ULL;
constexpr uint64_t SAG_CONSTANT_24_0 = 18446744069431361535ULL;
constexpr uint64_t SAG_CONSTANT_25_0 = 18446742978492891135ULL;
constexpr uint64_t SAG_CONSTANT_26_0 = 17868022691004938231ULL;
constexpr uint64_t SAG_CONSTANT_27_0 = 17289301308300324847ULL;
constexpr uint64_t SAG_CONSTANT_28_0 = 9205322385119247870ULL;
constexpr uint64_t SAG_CONSTANT_29_0 = 562984315256960ULL;
constexpr uint64_t SAG_CONSTANT_29_1 = 5764608072790056960ULL;
constexpr uint64_t SAG_CONSTANT_29_2 = 6052837899219501056ULL;
constexpr uint64_t SAG_CONSTANT_29_3 = 143552238122434559ULL;
constexpr uint64_t SAG_CONSTANT_30_0 = 18410644770238495741ULL;
constexpr uint64_t SAG_CONSTANT_31_0 = 18301494120373256191ULL;
constexpr uint64_t SAG_CONSTANT_32_0 = 13826033229414399743ULL;
constexpr uint64_t SAG_CONSTANT_33_0 = 18446460382394441663ULL;
constexpr uint64_t SAG_CONSTANT_34_0 = 18446603060805367803ULL;
constexpr uint64_t SAG_CONSTANT_35_0 = 16136388651561975807ULL;
constexpr uint64_t SAG_CONSTANT_36_0 = 18446742965540351967ULL;
constexpr uint64_t SAG_CONSTANT_37_0 = 18156244167037026303ULL;
constexpr uint64_t SAG_CONSTANT_38_0 = 18446743522877894647ULL;
constexpr uint64_t SAG_CONSTANT_39_0 = 17291566362635796479ULL;
constexpr uint64_t SAG_CONSTANT_40_0 = 18446744069380765679ULL;
constexpr uint64_t SAG_CONSTANT_41_0 = 17865744260381278207ULL;
constexpr uint64_t SAG_CONSTANT_42_0 = 18446744073692642295ULL;
constexpr uint64_t SAG_CONSTANT_43_0 = 17869155218181062655ULL;
constexpr uint64_t SAG_CONSTANT_44_0 = 18446744071557865455ULL;
constexpr uint64_t SAG_CONSTANT_45_0 = 17284744451347972095ULL;
void eval_set_pext_unrolled(int eval_feature[EVAL_N_FEATURE + 1], const uint64_t player, const uint64_t opponent) {
	const uint64_t empty = ~(player | opponent);
	alignas(32) uint16_t u8l8[16] = {};
	{ // for (int i = 0; i < 6; ++i) {
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 0
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_0_0) << 7) | _pext_u64(e, ~SAG_CONSTANT_0_0);
				o = (_pext_u64(o, SAG_CONSTANT_0_0) << 7) | _pext_u64(o, ~SAG_CONSTANT_0_0);
				e = (_pext_u64(e, SAG_CONSTANT_0_1) << 6) | _pext_u64(e, ~SAG_CONSTANT_0_1);
				o = (_pext_u64(o, SAG_CONSTANT_0_1) << 6) | _pext_u64(o, ~SAG_CONSTANT_0_1);
				u8l8[0] = BIT_REVERSE_TABLE[9][e & 511];
				u8l8[4] = BIT_REVERSE_TABLE[9][o & 511];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 1
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_1_0) << 59) | _pext_u64(e, ~SAG_CONSTANT_1_0);
				o = (_pext_u64(o, SAG_CONSTANT_1_0) << 59) | _pext_u64(o, ~SAG_CONSTANT_1_0);
				e = (_pext_u64(e, SAG_CONSTANT_1_1) << 6) | _pext_u64(e, ~SAG_CONSTANT_1_1);
				o = (_pext_u64(o, SAG_CONSTANT_1_1) << 6) | _pext_u64(o, ~SAG_CONSTANT_1_1);
				e = (_pext_u64(e, SAG_CONSTANT_1_2) << 6) | _pext_u64(e, ~SAG_CONSTANT_1_2);
				o = (_pext_u64(o, SAG_CONSTANT_1_2) << 6) | _pext_u64(o, ~SAG_CONSTANT_1_2);
				u8l8[1] = BIT_REVERSE_TABLE[9][e & 511];
				u8l8[5] = BIT_REVERSE_TABLE[9][o & 511];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 2
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_2_0) << 59) | _pext_u64(e, ~SAG_CONSTANT_2_0);
				o = (_pext_u64(o, SAG_CONSTANT_2_0) << 59) | _pext_u64(o, ~SAG_CONSTANT_2_0);
				e = (_pext_u64(e, SAG_CONSTANT_2_1) << 6) | _pext_u64(e, ~SAG_CONSTANT_2_1);
				o = (_pext_u64(o, SAG_CONSTANT_2_1) << 6) | _pext_u64(o, ~SAG_CONSTANT_2_1);
				e = (_pext_u64(e, SAG_CONSTANT_2_2) << 6) | _pext_u64(e, ~SAG_CONSTANT_2_2);
				o = (_pext_u64(o, SAG_CONSTANT_2_2) << 6) | _pext_u64(o, ~SAG_CONSTANT_2_2);
				u8l8[2] = BIT_REVERSE_TABLE[9][e & 511];
				u8l8[6] = BIT_REVERSE_TABLE[9][o & 511];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 3
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_3_0) << 59) | _pext_u64(e, ~SAG_CONSTANT_3_0);
				o = (_pext_u64(o, SAG_CONSTANT_3_0) << 59) | _pext_u64(o, ~SAG_CONSTANT_3_0);
				e = (_pext_u64(e, SAG_CONSTANT_3_1) << 6) | _pext_u64(e, ~SAG_CONSTANT_3_1);
				o = (_pext_u64(o, SAG_CONSTANT_3_1) << 6) | _pext_u64(o, ~SAG_CONSTANT_3_1);
				e = (_pext_u64(e, SAG_CONSTANT_3_2) << 6) | _pext_u64(e, ~SAG_CONSTANT_3_2);
				o = (_pext_u64(o, SAG_CONSTANT_3_2) << 6) | _pext_u64(o, ~SAG_CONSTANT_3_2);
				u8l8[3] = BIT_REVERSE_TABLE[9][e & 511];
				u8l8[7] = BIT_REVERSE_TABLE[9][o & 511];
			}
		}
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 4
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_4_0) << 4) | _pext_u64(e, ~SAG_CONSTANT_4_0);
				o = (_pext_u64(o, SAG_CONSTANT_4_0) << 4) | _pext_u64(o, ~SAG_CONSTANT_4_0);
				e = (_pext_u64(e, SAG_CONSTANT_4_1) << 62) | _pext_u64(e, ~SAG_CONSTANT_4_1);
				o = (_pext_u64(o, SAG_CONSTANT_4_1) << 62) | _pext_u64(o, ~SAG_CONSTANT_4_1);
				e = (_pext_u64(e, SAG_CONSTANT_4_2) << 4) | _pext_u64(e, ~SAG_CONSTANT_4_2);
				o = (_pext_u64(o, SAG_CONSTANT_4_2) << 4) | _pext_u64(o, ~SAG_CONSTANT_4_2);
				u8l8[8] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[12] = BIT_REVERSE_TABLE[10][o & 1023];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 5
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_5_0) << 6) | _pext_u64(e, ~SAG_CONSTANT_5_0);
				o = (_pext_u64(o, SAG_CONSTANT_5_0) << 6) | _pext_u64(o, ~SAG_CONSTANT_5_0);
				e = (_pext_u64(e, SAG_CONSTANT_5_1) << 60) | _pext_u64(e, ~SAG_CONSTANT_5_1);
				o = (_pext_u64(o, SAG_CONSTANT_5_1) << 60) | _pext_u64(o, ~SAG_CONSTANT_5_1);
				e = (_pext_u64(e, SAG_CONSTANT_5_2) << 59) | _pext_u64(e, ~SAG_CONSTANT_5_2);
				o = (_pext_u64(o, SAG_CONSTANT_5_2) << 59) | _pext_u64(o, ~SAG_CONSTANT_5_2);
				e = (_pext_u64(e, SAG_CONSTANT_5_3) << 9) | _pext_u64(e, ~SAG_CONSTANT_5_3);
				o = (_pext_u64(o, SAG_CONSTANT_5_3) << 9) | _pext_u64(o, ~SAG_CONSTANT_5_3);
				u8l8[9] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[13] = BIT_REVERSE_TABLE[10][o & 1023];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 6
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_6_0) << 59) | _pext_u64(e, ~SAG_CONSTANT_6_0);
				o = (_pext_u64(o, SAG_CONSTANT_6_0) << 59) | _pext_u64(o, ~SAG_CONSTANT_6_0);
				e = _pext_u64(e, ~SAG_CONSTANT_6_1);
				o = _pext_u64(o, ~SAG_CONSTANT_6_1);
				u8l8[10] = BIT_REVERSE_TABLE[10][e];
				u8l8[14] = BIT_REVERSE_TABLE[10][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 7
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_7_0) << 7) | _pext_u64(e, ~SAG_CONSTANT_7_0);
				o = (_pext_u64(o, SAG_CONSTANT_7_0) << 7) | _pext_u64(o, ~SAG_CONSTANT_7_0);
				e = (_pext_u64(e, SAG_CONSTANT_7_1) << 62) | _pext_u64(e, ~SAG_CONSTANT_7_1);
				o = (_pext_u64(o, SAG_CONSTANT_7_1) << 62) | _pext_u64(o, ~SAG_CONSTANT_7_1);
				e = (_pext_u64(e, SAG_CONSTANT_7_2) << 9) | _pext_u64(e, ~SAG_CONSTANT_7_2);
				o = (_pext_u64(o, SAG_CONSTANT_7_2) << 9) | _pext_u64(o, ~SAG_CONSTANT_7_2);
				u8l8[11] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[15] = BIT_REVERSE_TABLE[10][o & 1023];
			}
		}
		ternarize_avx2_full(u8l8, &eval_feature[0], &EVAL_OFFSET[0]);
	}
	{ // for (int i = 0; i < 6; ++i) {
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 8
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_8_0) << 55) | _pext_u64(e, ~SAG_CONSTANT_8_0);
				o = (_pext_u64(o, SAG_CONSTANT_8_0) << 55) | _pext_u64(o, ~SAG_CONSTANT_8_0);
				e = _pext_u64(e, ~SAG_CONSTANT_8_1);
				o = _pext_u64(o, ~SAG_CONSTANT_8_1);
				u8l8[0] = BIT_REVERSE_TABLE[10][e];
				u8l8[4] = BIT_REVERSE_TABLE[10][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 9
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_9_0) << 63) | _pext_u64(e, ~SAG_CONSTANT_9_0);
				o = (_pext_u64(o, SAG_CONSTANT_9_0) << 63) | _pext_u64(o, ~SAG_CONSTANT_9_0);
				e = _pext_u64(e, ~SAG_CONSTANT_9_1);
				o = _pext_u64(o, ~SAG_CONSTANT_9_1);
				u8l8[1] = BIT_REVERSE_TABLE[10][e];
				u8l8[5] = BIT_REVERSE_TABLE[10][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 10
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_10_0) << 2) | _pext_u64(e, ~SAG_CONSTANT_10_0);
				o = (_pext_u64(o, SAG_CONSTANT_10_0) << 2) | _pext_u64(o, ~SAG_CONSTANT_10_0);
				e = (_pext_u64(e, SAG_CONSTANT_10_1) << 9) | _pext_u64(e, ~SAG_CONSTANT_10_1);
				o = (_pext_u64(o, SAG_CONSTANT_10_1) << 9) | _pext_u64(o, ~SAG_CONSTANT_10_1);
				u8l8[2] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[6] = BIT_REVERSE_TABLE[10][o & 1023];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 11
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_11_0) << 2) | _pext_u64(e, ~SAG_CONSTANT_11_0);
				o = (_pext_u64(o, SAG_CONSTANT_11_0) << 2) | _pext_u64(o, ~SAG_CONSTANT_11_0);
				e = (_pext_u64(e, SAG_CONSTANT_11_1) << 9) | _pext_u64(e, ~SAG_CONSTANT_11_1);
				o = (_pext_u64(o, SAG_CONSTANT_11_1) << 9) | _pext_u64(o, ~SAG_CONSTANT_11_1);
				u8l8[3] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[7] = BIT_REVERSE_TABLE[10][o & 1023];
			}
		}
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 12
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_12_0) << 61) | _pext_u64(e, ~SAG_CONSTANT_12_0);
				o = (_pext_u64(o, SAG_CONSTANT_12_0) << 61) | _pext_u64(o, ~SAG_CONSTANT_12_0);
				e = _pext_u64(e, ~SAG_CONSTANT_12_1);
				o = _pext_u64(o, ~SAG_CONSTANT_12_1);
				u8l8[8] = BIT_REVERSE_TABLE[10][e];
				u8l8[12] = BIT_REVERSE_TABLE[10][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 13
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_13_0) << 57) | _pext_u64(e, ~SAG_CONSTANT_13_0);
				o = (_pext_u64(o, SAG_CONSTANT_13_0) << 57) | _pext_u64(o, ~SAG_CONSTANT_13_0);
				e = _pext_u64(e, ~SAG_CONSTANT_13_1);
				o = _pext_u64(o, ~SAG_CONSTANT_13_1);
				u8l8[9] = BIT_REVERSE_TABLE[10][e];
				u8l8[13] = BIT_REVERSE_TABLE[10][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 14
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_14_0) << 6) | _pext_u64(e, ~SAG_CONSTANT_14_0);
				o = (_pext_u64(o, SAG_CONSTANT_14_0) << 6) | _pext_u64(o, ~SAG_CONSTANT_14_0);
				e = (_pext_u64(e, SAG_CONSTANT_14_1) << 7) | _pext_u64(e, ~SAG_CONSTANT_14_1);
				o = (_pext_u64(o, SAG_CONSTANT_14_1) << 7) | _pext_u64(o, ~SAG_CONSTANT_14_1);
				u8l8[10] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[14] = BIT_REVERSE_TABLE[10][o & 1023];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 15
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_15_0) << 6) | _pext_u64(e, ~SAG_CONSTANT_15_0);
				o = (_pext_u64(o, SAG_CONSTANT_15_0) << 6) | _pext_u64(o, ~SAG_CONSTANT_15_0);
				e = (_pext_u64(e, SAG_CONSTANT_15_1) << 7) | _pext_u64(e, ~SAG_CONSTANT_15_1);
				o = (_pext_u64(o, SAG_CONSTANT_15_1) << 7) | _pext_u64(o, ~SAG_CONSTANT_15_1);
				u8l8[11] = BIT_REVERSE_TABLE[10][e & 1023];
				u8l8[15] = BIT_REVERSE_TABLE[10][o & 1023];
			}
		}
		ternarize_avx2_full(u8l8, &eval_feature[8], &EVAL_OFFSET[8]);
	}
	{ // for (int i = 0; i < 6; ++i) {
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 16
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_16_0);
				o = _pext_u64(o, ~SAG_CONSTANT_16_0);
				u8l8[0] = BIT_REVERSE_TABLE[8][e];
				u8l8[4] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 17
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_17_0);
				o = _pext_u64(o, ~SAG_CONSTANT_17_0);
				u8l8[1] = BIT_REVERSE_TABLE[8][e];
				u8l8[5] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 18
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_18_0);
				o = _pext_u64(o, ~SAG_CONSTANT_18_0);
				u8l8[2] = BIT_REVERSE_TABLE[8][e];
				u8l8[6] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 19
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_19_0);
				o = _pext_u64(o, ~SAG_CONSTANT_19_0);
				u8l8[3] = BIT_REVERSE_TABLE[8][e];
				u8l8[7] = BIT_REVERSE_TABLE[8][o];
			}
		}
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 20
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_20_0);
				o = _pext_u64(o, ~SAG_CONSTANT_20_0);
				u8l8[8] = BIT_REVERSE_TABLE[8][e];
				u8l8[12] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 21
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_21_0);
				o = _pext_u64(o, ~SAG_CONSTANT_21_0);
				u8l8[9] = BIT_REVERSE_TABLE[8][e];
				u8l8[13] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 22
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_22_0);
				o = _pext_u64(o, ~SAG_CONSTANT_22_0);
				u8l8[10] = BIT_REVERSE_TABLE[8][e];
				u8l8[14] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 23
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_23_0);
				o = _pext_u64(o, ~SAG_CONSTANT_23_0);
				u8l8[11] = BIT_REVERSE_TABLE[8][e];
				u8l8[15] = BIT_REVERSE_TABLE[8][o];
			}
		}
		ternarize_avx2_full(u8l8, &eval_feature[16], &EVAL_OFFSET[16]);
	}
	{ // for (int i = 0; i < 6; ++i) {
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 24
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_24_0);
				o = _pext_u64(o, ~SAG_CONSTANT_24_0);
				u8l8[0] = BIT_REVERSE_TABLE[8][e];
				u8l8[4] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 25
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_25_0);
				o = _pext_u64(o, ~SAG_CONSTANT_25_0);
				u8l8[1] = BIT_REVERSE_TABLE[8][e];
				u8l8[5] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 26
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_26_0);
				o = _pext_u64(o, ~SAG_CONSTANT_26_0);
				u8l8[2] = BIT_REVERSE_TABLE[8][e];
				u8l8[6] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 27
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_27_0);
				o = _pext_u64(o, ~SAG_CONSTANT_27_0);
				u8l8[3] = BIT_REVERSE_TABLE[8][e];
				u8l8[7] = BIT_REVERSE_TABLE[8][o];
			}
		}
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 28
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_28_0);
				o = _pext_u64(o, ~SAG_CONSTANT_28_0);
				u8l8[8] = BIT_REVERSE_TABLE[8][e];
				u8l8[12] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 29
				uint64_t e = empty;
				uint64_t o = opponent;
				e = (_pext_u64(e, SAG_CONSTANT_29_0) << 60) | _pext_u64(e, ~SAG_CONSTANT_29_0);
				o = (_pext_u64(o, SAG_CONSTANT_29_0) << 60) | _pext_u64(o, ~SAG_CONSTANT_29_0);
				e = (_pext_u64(e, SAG_CONSTANT_29_1) << 60) | _pext_u64(e, ~SAG_CONSTANT_29_1);
				o = (_pext_u64(o, SAG_CONSTANT_29_1) << 60) | _pext_u64(o, ~SAG_CONSTANT_29_1);
				e = (_pext_u64(e, SAG_CONSTANT_29_2) << 60) | _pext_u64(e, ~SAG_CONSTANT_29_2);
				o = (_pext_u64(o, SAG_CONSTANT_29_2) << 60) | _pext_u64(o, ~SAG_CONSTANT_29_2);
				e = _pext_u64(e, ~SAG_CONSTANT_29_3);
				o = _pext_u64(o, ~SAG_CONSTANT_29_3);
				u8l8[9] = BIT_REVERSE_TABLE[8][e];
				u8l8[13] = BIT_REVERSE_TABLE[8][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 30
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_30_0);
				o = _pext_u64(o, ~SAG_CONSTANT_30_0);
				u8l8[10] = BIT_REVERSE_TABLE[7][e];
				u8l8[14] = BIT_REVERSE_TABLE[7][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 31
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_31_0);
				o = _pext_u64(o, ~SAG_CONSTANT_31_0);
				u8l8[11] = BIT_REVERSE_TABLE[7][e];
				u8l8[15] = BIT_REVERSE_TABLE[7][o];
			}
		}
		ternarize_avx2_full(u8l8, &eval_feature[24], &EVAL_OFFSET[24]);
	}
	{ // for (int i = 0; i < 6; ++i) {
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 32
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_32_0);
				o = _pext_u64(o, ~SAG_CONSTANT_32_0);
				u8l8[0] = BIT_REVERSE_TABLE[7][e];
				u8l8[4] = BIT_REVERSE_TABLE[7][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 33
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_33_0);
				o = _pext_u64(o, ~SAG_CONSTANT_33_0);
				u8l8[1] = BIT_REVERSE_TABLE[7][e];
				u8l8[5] = BIT_REVERSE_TABLE[7][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 34
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_34_0);
				o = _pext_u64(o, ~SAG_CONSTANT_34_0);
				u8l8[2] = BIT_REVERSE_TABLE[6][e];
				u8l8[6] = BIT_REVERSE_TABLE[6][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 35
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_35_0);
				o = _pext_u64(o, ~SAG_CONSTANT_35_0);
				u8l8[3] = BIT_REVERSE_TABLE[6][e];
				u8l8[7] = BIT_REVERSE_TABLE[6][o];
			}
		}
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 36
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_36_0);
				o = _pext_u64(o, ~SAG_CONSTANT_36_0);
				u8l8[8] = BIT_REVERSE_TABLE[6][e];
				u8l8[12] = BIT_REVERSE_TABLE[6][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 37
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_37_0);
				o = _pext_u64(o, ~SAG_CONSTANT_37_0);
				u8l8[9] = BIT_REVERSE_TABLE[6][e];
				u8l8[13] = BIT_REVERSE_TABLE[6][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 38
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_38_0);
				o = _pext_u64(o, ~SAG_CONSTANT_38_0);
				u8l8[10] = BIT_REVERSE_TABLE[5][e];
				u8l8[14] = BIT_REVERSE_TABLE[5][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 39
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_39_0);
				o = _pext_u64(o, ~SAG_CONSTANT_39_0);
				u8l8[11] = BIT_REVERSE_TABLE[5][e];
				u8l8[15] = BIT_REVERSE_TABLE[5][o];
			}
		}
		ternarize_avx2_full(u8l8, &eval_feature[32], &EVAL_OFFSET[32]);
	}
	{ // for (int i = 0; i < 6; ++i) {
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 40
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_40_0);
				o = _pext_u64(o, ~SAG_CONSTANT_40_0);
				u8l8[0] = BIT_REVERSE_TABLE[5][e];
				u8l8[4] = BIT_REVERSE_TABLE[5][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 41
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_41_0);
				o = _pext_u64(o, ~SAG_CONSTANT_41_0);
				u8l8[1] = BIT_REVERSE_TABLE[5][e];
				u8l8[5] = BIT_REVERSE_TABLE[5][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 42
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_42_0);
				o = _pext_u64(o, ~SAG_CONSTANT_42_0);
				u8l8[2] = BIT_REVERSE_TABLE[4][e];
				u8l8[6] = BIT_REVERSE_TABLE[4][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 43
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_43_0);
				o = _pext_u64(o, ~SAG_CONSTANT_43_0);
				u8l8[3] = BIT_REVERSE_TABLE[4][e];
				u8l8[7] = BIT_REVERSE_TABLE[4][o];
			}
		}
		{ // for (int j = 0; j < 2; ++j) {
			{ // for (int k = 0; k < 4; ++k) {
			// index = 44
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_44_0);
				o = _pext_u64(o, ~SAG_CONSTANT_44_0);
				u8l8[8] = BIT_REVERSE_TABLE[4][e];
				u8l8[12] = BIT_REVERSE_TABLE[4][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 45
				uint64_t e = empty;
				uint64_t o = opponent;
				e = _pext_u64(e, ~SAG_CONSTANT_45_0);
				o = _pext_u64(o, ~SAG_CONSTANT_45_0);
				u8l8[9] = BIT_REVERSE_TABLE[4][e];
				u8l8[13] = BIT_REVERSE_TABLE[4][o];
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 46
				u8l8[10] = 0;
				u8l8[14] = 0;
			}
			{ // for (int k = 0; k < 4; ++k) {
			// index = 47
				u8l8[11] = 0;
				u8l8[15] = 0;
			}
		}
		ternarize_avx2_full(u8l8, &eval_feature[40], &EVAL_OFFSET[40]);
	}
}
