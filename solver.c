// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2025 Marc Gonzalez

#include <stdio.h>
#include <stdlib.h>

#if BENCHMARK || NDEBUG
#define printf(...) // Turn printf into a NOP
#endif

typedef unsigned idx;
typedef unsigned long val;

// GCC already optimizes divide-by-constant into mul+shr.
// We will only use div3 for [0-9] and div9 for [0-81].
// Using smaller multipliers enables further optimizations.

// Round 0-15 down to nearest power of 2
#define RND(u) (((u>>0 | u>>1 | u>>2 | u>>3) + 1) / 2)
// HACK: No parentheses around expansion to compute u*num first
#define AUX(d,pow2) (((pow2)+d-1) / d) / (pow2)
#define MAGIC_OP(d,log2) AUX(d, RND(d)<<log2)
static inline idx div3(idx u) { return u * MAGIC_OP(3,4); } // u * 11 >> 5
static inline idx div9(idx u) { return u * MAGIC_OP(9,6); } // u * 57 >> 9

// Store solution 's' as 1 << s*4
// Using a bitmask allows oring different bitmasks together,
// to check rows, blocks, lines in a single operation (no loops).
// We need 4 bits per solution to be able to sum over 9 cells,
// to check if a solution is invalid in 8 cells.

typedef struct {
	val M[9*9], R[9], C[9], B[9];
	idx found, depth;
} ctx_t;

#define M ctx->M
#define R ctx->R
#define C ctx->C
#define B ctx->B

#define ALL (val)0x111111111

static inline val sol_to_val(idx s) { return (val)1 << (s << 2); }
static inline idx val_to_sol(val v) { return __builtin_ctzl(v) >> 2; }
static inline int valid_sym(char c) { return '1' <= c && c <= '9'; }
static inline val sym_to_val(char c) { return sol_to_val(c - '1'); }
static inline char val_to_sym(val v) { return '1' + val_to_sol(v); }

static void set_val(ctx_t *ctx, val v, val r, val c, val b, idx i, idx j, idx k, idx n)
{
	M[n] = v;
	R[i] = r | v;
	C[j] = c | v;
	B[k] = b | v;
	++ctx->found;
}

static void read_grid(ctx_t *ctx)
{
	char buf[9][9+1];
	if (!fread(buf, sizeof buf, 1, stdin)) return;

	for (idx i = 0; i < 9; ++i)
	{
		for (idx j = 0; j < 9; ++j)
		{
			char c = buf[i][j];
			if (valid_sym(c))
			{
				val v = sym_to_val(c);
				idx k = div3(i)*3 + div3(j);
				set_val(ctx, v, R[i], C[j], B[k], i, j, k, i*9+j);
			}
		}
	}
}

#ifdef BENCHMARK
static void print_grid(ctx_t *ctx) { }
#else
static void print_grid(ctx_t *ctx)
{
	char buf[9][9+1];
	for (idx i = 0; i < 9; ++i)
	{
		for (idx j = 0; j < 9; ++j)
			buf[i][j] = val_to_sym(M[i*9+j]);
		buf[i][9] = '\n';
	}
	fwrite(buf, sizeof buf, 1, stdout);
}
#endif

static idx calls __attribute__ ((unused));
static void find_solutions(ctx_t *ctx, idx n);
static void test_solutions(ctx_t *ctx, idx n)
{
#ifndef BENCHMARK
	++calls;
	++ctx->depth;
#endif

	while (M[n] != 0) ++n;

	idx i = div9(n);
	idx j = n-i*9;
	idx k = div3(i)*3 + div3(j);
	val r = R[i], c = C[j], b = B[k];
	val m = ~(r|c|b) & ALL;

	if (m == 0) return;

	ctx_t backup = *ctx;

	// Try every valid solution then backtrack
	do {
		idx s = val_to_sol(m);
		val v = sol_to_val(s);
		m ^= v;
		printf("TRY M(%u,%u) = %u AT DEPTH=%u\n", i+1, j+1, s+1, ctx->depth);
		set_val(ctx, v, r, c, b, i, j, k, n);
		find_solutions(ctx, n+1);
		*ctx = backup;
	} while (m != 0);
}

// For every cell, compute mask of invalid solutions.
// If a cell has 8 invalid solutions, only 1 solution remains valid.
static void check_cells(ctx_t *ctx)
{
	for (idx i = 0; i < 9; ++i)
	{
		for (idx j = 0; j < 9; ++j)
		{
			if (M[i*9+j] == 0)
			{
				idx k = div3(i)*3 + div3(j);
				val r = R[i], c = C[j], b = B[k];
				val m = r|c|b;
				if (__builtin_popcountl(m) == 8)
				{
					val v = ~m & ALL;
					set_val(ctx, v, r, c, b, i, j, k, i*9+j);
					printf("CC: M(%u,%u) = %c\n", i+1, j+1, val_to_sym(v));
				}
			}
		}
	}
}

// For every block, compute mask of counts of invalid positions.
// If a solution has 8 invalid positions, we can scan the block
// to find the only valid position.
static void check_blocks(ctx_t *ctx)
{
	val BB[9] = { 0 };

	for (idx i = 0; i < 9; ++i)
		for (idx j = 0; j < 9; ++j)
		{
			val m = ALL;
			idx k = div3(i)*3 + div3(j);
			if (M[i*9+j] == 0) m = R[i] | C[j] | B[k];
			BB[k] += m;
		}

// ~B[k] = mask of unknown solutions (9s in BB)
// 1) BB[k] >> 3 & ALL maps 0-7 to 0 & 8-9 to 1
// 2) Masking with ~B[k] maps 9 to 0
// 3) Remaining 1s denote solutions with 8 invalid positions.
// For example, consider BB[k] = 0x947938568
// 1) 0x947938568 => 0x100101001
// 2) 0x100101001 => 0x000001001
// 3) Solutions 1 & 4 have only 1 valid position
	for (idx k = 0; k < 9; ++k)
	{
		val vv = (BB[k] >> 3) & (ALL & ~B[k]);
		while (vv != 0)
		{
			idx s = val_to_sol(vv);
			val v = sol_to_val(s);
			vv ^= v;
			for (idx z = 0; z < 9; ++z)
			{
				idx ii = div3(k)*3; idx jj = (k-ii)*3;
				idx zdiv3 = div3(z); idx zmod3 = z-zdiv3*3;
				idx i = ii + zdiv3; idx j = jj + zmod3;
				idx n = i*9+j;
				if (M[n] == 0)
				{
					val r = R[i], c = C[j], b = B[k];
					val m = r|c|b;
					if ((m & v) == 0)
					{
						set_val(ctx, v, r, c, b, i, j, k, n);
						printf("B%d: M(%d,%d) = %d\n", k+1, i+1, j+1, s+1);
						break;
					}
				}
			}
		}
	}
}

static void find_solutions(ctx_t *ctx, idx n)
{
	idx prev, curr, it = 0;

	do {
		++it;
		prev = ctx->found;
		check_cells(ctx);
		check_blocks(ctx);
		curr = ctx->found;
		printf("IT=%u PREV=%u CURR=%u\n", it, prev, curr);
		if (curr == prev)
		{
			test_solutions(ctx, n);
			return;
		}
	} while (curr < 9*9);

	print_grid(ctx);
}

static ctx_t *bak;

void init(void)
{
	bak = malloc(sizeof *bak);
	read_grid(bak);
}

void spin(void)
{
	ctx_t ctx = *bak;
	find_solutions(&ctx, 0);
}

#ifndef BENCHMARK
int main(void)
{
	init();
	spin();
	printf("CALLS=%u\n", calls);
	return 0;
}
#endif
