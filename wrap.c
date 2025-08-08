// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2025 Marc Gonzalez

#include <stdlib.h>
#include <stdio.h>

typedef unsigned int u32;

// Ignore id: process will be pinned to core 3
// Ignore hi: t1-t0 < 2^32 cycles (1.3 seconds @ 3.3 GHz)
static inline u32 rdtscp(void)
{
	u32 hi, lo, id;
	asm volatile("rdtscp" : "=d"(hi), "=a"(lo), "=c"(id));
	return lo;
}

static inline void serialize(void)
{
	u32 eax, ebx, ecx, edx;
	asm volatile("xor %0, %0; cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
}

extern void init(void);
extern void spin(void);

static u32 time_spin(void)
{
	serialize();
	u32 t0 = rdtscp();
	spin();
	u32 t1 = rdtscp();
	serialize();
	return t1 - t0;
}

static void print_all(u32 N)
{
	u32 *buf = aligned_alloc(4096, N * sizeof *buf);

	for (u32 n = 0; n < N; n += 4)
	{
		u32 v[4];
		v[0] = time_spin();
		v[1] = time_spin();
		v[2] = time_spin();
		v[3] = time_spin();
		__builtin_ia32_movntdq((void *)(buf + n), __builtin_ia32_movntdqa((void *)v));
	}

	for (u32 n = 0; n < N; ++n) printf("%u\n", buf[n]);

	free(buf);
}

static void print_min(u32 N)
{
	u32 min = -1;

	for (u32 n = 0; n < N; ++n)
	{
		u32 curr = time_spin();
		if (curr < min) min = curr;
	}

	printf("%u\n", min);
}

int main(int argc, char **argv)
{
	if (argc < 2) return 1;

	init();

	// Round N down to multiple of 4
	u32 N = strtoul(argv[1], NULL, 10) / 4 * 4;
	if (argc > 2) print_all(N); else print_min(N);

	return 0;
}
