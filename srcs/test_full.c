/* ************************************************************************** */
/*                                                                            */
/*   Testeur exhaustif pour ft_malloc / ft_free / ft_realloc                  */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>
#include <stdint.h>
#include "../includes/ft_malloc.h"

/* ===== COLORS ===== */
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define CYAN "\033[0;36m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

/* ===== COUNTERS ===== */
static int g_pass = 0;
static int g_fail = 0;
static int g_total = 0;

/* ===== SIGNAL HANDLING FOR CRASH DETECTION ===== */
static sigjmp_buf g_jump_buf;
static volatile sig_atomic_t g_crash_expected = 0;

static void crash_handler(int sig)
{
	(void)sig;
	if (g_crash_expected)
		siglongjmp(g_jump_buf, 1);
	/* If not expected, let it crash normally */
	signal(sig, SIG_DFL);
	raise(sig);
}

/* Run a block of code, return 1 if it crashed (SEGV/BUS/ABRT) */
#define EXPECT_NO_CRASH(code)              \
	do                                     \
	{                                      \
		g_crash_expected = 1;              \
		if (sigsetjmp(g_jump_buf, 1) == 0) \
		{                                  \
			code;                          \
			g_crash_expected = 0;          \
		}                                  \
		else                               \
		{                                  \
			g_crash_expected = 0;          \
			CHECK(0, "CRASHED");           \
		}                                  \
	} while (0)

/* ===== TEST MACROS ===== */
#define CHECK(cond, msg)                                 \
	do                                                   \
	{                                                    \
		g_total++;                                       \
		if (cond)                                        \
		{                                                \
			g_pass++;                                    \
			printf(GREEN "  [PASS] " RESET "%s\n", msg); \
		}                                                \
		else                                             \
		{                                                \
			g_fail++;                                    \
			printf(RED "  [FAIL] " RESET "%s\n", msg);   \
		}                                                \
	} while (0)

#define SECTION(name) printf("\n" BOLD CYAN "══════ %s ══════" RESET "\n", name)

/* ===== HELPER: fill memory with pattern and verify ===== */
static void fill_pattern(void *ptr, size_t size, unsigned char seed)
{
	unsigned char *p = (unsigned char *)ptr;
	for (size_t i = 0; i < size; i++)
		p[i] = (unsigned char)((seed + i) & 0xFF);
}

static int verify_pattern(void *ptr, size_t size, unsigned char seed)
{
	unsigned char *p = (unsigned char *)ptr;
	for (size_t i = 0; i < size; i++)
	{
		if (p[i] != (unsigned char)((seed + i) & 0xFF))
			return (0);
	}
	return (1);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/*                           TEST FUNCTIONS                                  */
/* ═══════════════════════════════════════════════════════════════════════════ */

/* ----- 1. Basic malloc ----- */
static void test_basic_malloc(void)
{
	SECTION("1. BASIC MALLOC");
	void *p;

	p = ft_malloc(1);
	CHECK(p != NULL, "malloc(1) returns non-NULL");
	ft_free(p);

	p = ft_malloc(10);
	CHECK(p != NULL, "malloc(10) returns non-NULL");
	ft_free(p);

	p = ft_malloc(100);
	CHECK(p != NULL, "malloc(100) returns non-NULL");
	ft_free(p);

	p = ft_malloc(500);
	CHECK(p != NULL, "malloc(500) returns non-NULL");
	ft_free(p);

	p = ft_malloc(992);
	CHECK(p != NULL, "malloc(992) returns non-NULL (max arena)");
	ft_free(p);
}

/* ----- 2. malloc(0) ----- */
static void test_malloc_zero(void)
{
	SECTION("2. MALLOC(0)");
	void *p = ft_malloc(0);
	CHECK(p == NULL, "malloc(0) returns NULL");
}

/* ----- 3. Big allocations (>1024, uses mmap) ----- */
static void test_big_alloc(void)
{
	SECTION("3. BIG ALLOCATIONS (mmap)");
	void *p;

	p = ft_malloc(1025);
	CHECK(p != NULL, "malloc(1025) returns non-NULL");
	fill_pattern(p, 1025, 0xAA);
	CHECK(verify_pattern(p, 1025, 0xAA), "malloc(1025) data integrity");
	ft_free(p);

	p = ft_malloc(4096);
	CHECK(p != NULL, "malloc(4096) returns non-NULL");
	fill_pattern(p, 4096, 0xBB);
	CHECK(verify_pattern(p, 4096, 0xBB), "malloc(4096) data integrity");
	ft_free(p);

	p = ft_malloc(65536);
	CHECK(p != NULL, "malloc(65536) returns non-NULL");
	fill_pattern(p, 65536, 0xCC);
	CHECK(verify_pattern(p, 65536, 0xCC), "malloc(65536) data integrity");
	ft_free(p);

	p = ft_malloc(1024 * 1024);
	CHECK(p != NULL, "malloc(1MB) returns non-NULL");
	fill_pattern(p, 1024 * 1024, 0xDD);
	CHECK(verify_pattern(p, 1024 * 1024, 0xDD), "malloc(1MB) data integrity");
	ft_free(p);

	p = ft_malloc(10 * 1024 * 1024);
	CHECK(p != NULL, "malloc(10MB) returns non-NULL");
	ft_free(p);
}

/* ----- 4. free(NULL) ----- */
static void test_free_null(void)
{
	SECTION("4. FREE(NULL)");
	EXPECT_NO_CRASH(ft_free(NULL));
	CHECK(1, "free(NULL) does not crash");
}

/* ----- 5. Double free ----- */
static void test_double_free(void)
{
	SECTION("5. DOUBLE FREE");
	void *p = ft_malloc(42);
	CHECK(p != NULL, "malloc(42) returns non-NULL");
	ft_free(p);
	EXPECT_NO_CRASH(ft_free(p));
	CHECK(1, "double free does not crash (small block)");

	void *big = ft_malloc(2000);
	CHECK(big != NULL, "malloc(2000) returns non-NULL");
	ft_free(big);
	EXPECT_NO_CRASH(ft_free(big));
	CHECK(1, "double free does not crash (big block)");
}

/* ----- 6. Write & read integrity for all bucket sizes ----- */
static void test_write_read_integrity(void)
{
	SECTION("6. WRITE/READ INTEGRITY (all bucket sizes)");

	/* Effective usable sizes: 32-16=16, 64-16=48, 128-16=112, 256-16=240, 512-16=496, 1024-16=992 */
	size_t sizes[] = {1, 8, 15, 16, 30, 48, 100, 112, 200, 240, 400, 496, 800, 992};
	int nsizes = sizeof(sizes) / sizeof(sizes[0]);

	for (int i = 0; i < nsizes; i++)
	{
		void *p = ft_malloc(sizes[i]);
		char msg[128];
		snprintf(msg, sizeof(msg), "write/read integrity size=%zu", sizes[i]);
		if (p)
		{
			fill_pattern(p, sizes[i], (unsigned char)(i * 37));
			CHECK(verify_pattern(p, sizes[i], (unsigned char)(i * 37)), msg);
		}
		else
			CHECK(0, msg);
		ft_free(p);
	}
}

/* ----- 7. Multiple allocations don't overlap ----- */
static void test_no_overlap(void)
{
	SECTION("7. NO OVERLAP BETWEEN ALLOCATIONS");
#define NOVERLAP 200
	void *ptrs[NOVERLAP];
	size_t sizes[NOVERLAP];
	int ok = 1;

	for (int i = 0; i < NOVERLAP; i++)
	{
		sizes[i] = (i % 50) + 1; /* 1..50 */
		ptrs[i] = ft_malloc(sizes[i]);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
		fill_pattern(ptrs[i], sizes[i], (unsigned char)i);
	}

	/* Verify all patterns still intact */
	for (int i = 0; i < NOVERLAP && ok; i++)
	{
		if (!verify_pattern(ptrs[i], sizes[i], (unsigned char)i))
		{
			ok = 0;
			printf(RED "    overlap detected at allocation %d (size=%zu)" RESET "\n", i, sizes[i]);
		}
	}
	CHECK(ok, "200 allocations: no data overlap detected");

	for (int i = 0; i < NOVERLAP; i++)
		ft_free(ptrs[i]);
}

/* ----- 8. Realloc basics ----- */
static void test_realloc_basic(void)
{
	SECTION("8. REALLOC BASICS");

	/* realloc(NULL, size) == malloc(size) */
	void *p = ft_realloc(NULL, 42);
	CHECK(p != NULL, "realloc(NULL, 42) returns non-NULL (== malloc)");
	ft_free(p);

	/* realloc(ptr, 0) == free(ptr), returns NULL */
	p = ft_malloc(42);
	void *r = ft_realloc(p, 0);
	CHECK(r == NULL, "realloc(ptr, 0) returns NULL (== free)");

	/* realloc(NULL, 0) */
	r = ft_realloc(NULL, 0);
	CHECK(r == NULL, "realloc(NULL, 0) returns NULL");
}

/* ----- 9. Realloc grow ----- */
static void test_realloc_grow(void)
{
	SECTION("9. REALLOC GROW (data preservation)");

	void *p = ft_malloc(10);
	CHECK(p != NULL, "initial malloc(10)");
	fill_pattern(p, 10, 0x42);

	void *p2 = ft_realloc(p, 100);
	CHECK(p2 != NULL, "realloc to 100");
	CHECK(verify_pattern(p2, 10, 0x42), "data preserved after grow (10 -> 100)");
	ft_free(p2);

	/* Grow from small to big (arena -> mmap) */
	p = ft_malloc(50);
	CHECK(p != NULL, "initial malloc(50)");
	fill_pattern(p, 50, 0x55);

	p2 = ft_realloc(p, 5000);
	CHECK(p2 != NULL, "realloc to 5000 (small -> big)");
	CHECK(verify_pattern(p2, 50, 0x55), "data preserved after small->big grow");
	ft_free(p2);

	/* Grow big to bigger */
	p = ft_malloc(2000);
	CHECK(p != NULL, "initial malloc(2000)");
	fill_pattern(p, 2000, 0x77);

	p2 = ft_realloc(p, 10000);
	CHECK(p2 != NULL, "realloc 2000 -> 10000 (big -> bigger)");
	CHECK(verify_pattern(p2, 2000, 0x77), "data preserved after big->bigger grow");
	ft_free(p2);
}

/* ----- 10. Realloc shrink ----- */
static void test_realloc_shrink(void)
{
	SECTION("10. REALLOC SHRINK");

	void *p = ft_malloc(200);
	CHECK(p != NULL, "initial malloc(200)");
	fill_pattern(p, 200, 0x99);

	void *p2 = ft_realloc(p, 10);
	CHECK(p2 != NULL, "realloc 200 -> 10 (shrink)");
	/* When shrinking, if same bucket, ptr should be returned as-is */
	/* Data should be preserved for the smaller size */
	CHECK(verify_pattern(p2, 10, 0x99), "data preserved after shrink");
	ft_free(p2);
}

/* ----- 11. Stress test: many small allocs ----- */
static void test_stress_small(void)
{
	SECTION("11. STRESS TEST: MANY SMALL ALLOCATIONS");
#define NSTRESS 5000
	void *ptrs[NSTRESS];
	int all_ok = 1;

	for (int i = 0; i < NSTRESS; i++)
	{
		ptrs[i] = ft_malloc(16);
		if (!ptrs[i])
		{
			all_ok = 0;
			break;
		}
		memset(ptrs[i], 'A' + (i % 26), 16);
	}
	CHECK(all_ok, "5000 x malloc(16) all succeeded");

	/* Verify */
	int verify_ok = 1;
	for (int i = 0; i < NSTRESS && all_ok; i++)
	{
		unsigned char *p = (unsigned char *)ptrs[i];
		for (int j = 0; j < 16; j++)
		{
			if (p[j] != (unsigned char)('A' + (i % 26)))
			{
				verify_ok = 0;
				break;
			}
		}
		if (!verify_ok)
			break;
	}
	CHECK(verify_ok, "5000 allocations data integrity verified");

	for (int i = 0; i < NSTRESS; i++)
		ft_free(ptrs[i]);
	CHECK(1, "5000 frees completed");
}

/* ----- 12. Stress test: mixed sizes ----- */
static void test_stress_mixed(void)
{
	SECTION("12. STRESS TEST: MIXED SIZES");
#define NMIXED 2000
	void *ptrs[NMIXED];
	size_t sizes_arr[NMIXED];
	size_t possible[] = {1, 5, 10, 16, 32, 48, 64, 100, 128, 200, 256, 400, 500, 800, 992, 1025, 2000, 4096, 8000};
	int npossible = sizeof(possible) / sizeof(possible[0]);
	int all_ok = 1;

	for (int i = 0; i < NMIXED; i++)
	{
		sizes_arr[i] = possible[i % npossible];
		ptrs[i] = ft_malloc(sizes_arr[i]);
		if (!ptrs[i])
		{
			all_ok = 0;
			break;
		}
		fill_pattern(ptrs[i], sizes_arr[i], (unsigned char)i);
	}
	CHECK(all_ok, "2000 mixed-size allocations succeeded");

	int verify_ok = 1;
	for (int i = 0; i < NMIXED && all_ok; i++)
	{
		if (!verify_pattern(ptrs[i], sizes_arr[i], (unsigned char)i))
		{
			verify_ok = 0;
			printf(RED "    corruption at i=%d size=%zu" RESET "\n", i, sizes_arr[i]);
		}
	}
	CHECK(verify_ok, "2000 mixed-size data integrity verified");

	for (int i = 0; i < NMIXED; i++)
		ft_free(ptrs[i]);
	CHECK(1, "2000 mixed frees completed");
}

/* ----- 13. Free in reverse order ----- */
static void test_free_reverse(void)
{
	SECTION("13. FREE IN REVERSE ORDER (LIFO)");
#define NREV 500
	void *ptrs[NREV];
	int all_ok = 1;

	for (int i = 0; i < NREV; i++)
	{
		ptrs[i] = ft_malloc(30);
		if (!ptrs[i])
		{
			all_ok = 0;
			break;
		}
		fill_pattern(ptrs[i], 30, (unsigned char)i);
	}

	for (int i = 0; i < NREV && all_ok; i++)
	{
		if (!verify_pattern(ptrs[i], 30, (unsigned char)i))
			all_ok = 0;
	}
	CHECK(all_ok, "500 allocs data intact before reverse free");

	for (int i = NREV - 1; i >= 0; i--)
		ft_free(ptrs[i]);
	CHECK(1, "500 reverse frees completed");
}

/* ----- 14. Free every other (fragmentation) ----- */
static void test_fragmentation(void)
{
	SECTION("14. FRAGMENTATION (free every other)");
#define NFRAG 1000
	void *ptrs[NFRAG];
	int all_ok = 1;

	for (int i = 0; i < NFRAG; i++)
	{
		ptrs[i] = ft_malloc(20);
		if (!ptrs[i])
		{
			all_ok = 0;
			break;
		}
		fill_pattern(ptrs[i], 20, (unsigned char)i);
	}
	CHECK(all_ok, "1000 allocations for fragmentation test");

	/* Free even indices */
	for (int i = 0; i < NFRAG; i += 2)
	{
		ft_free(ptrs[i]);
		ptrs[i] = NULL;
	}

	/* Verify odd indices still intact */
	int intact = 1;
	for (int i = 1; i < NFRAG; i += 2)
	{
		if (!verify_pattern(ptrs[i], 20, (unsigned char)i))
		{
			intact = 0;
			printf(RED "    data corruption at index %d after fragmentation" RESET "\n", i);
			break;
		}
	}
	CHECK(intact, "remaining blocks intact after fragmentation");

	/* Re-allocate in the holes */
	int realloc_ok = 1;
	for (int i = 0; i < NFRAG; i += 2)
	{
		ptrs[i] = ft_malloc(20);
		if (!ptrs[i])
		{
			realloc_ok = 0;
			break;
		}
		fill_pattern(ptrs[i], 20, (unsigned char)(i + 100));
	}
	CHECK(realloc_ok, "re-allocation in holes succeeded");

	/* Verify everything */
	int all_intact = 1;
	for (int i = 0; i < NFRAG; i++)
	{
		unsigned char seed = (i % 2 == 0) ? (unsigned char)(i + 100) : (unsigned char)i;
		if (!verify_pattern(ptrs[i], 20, seed))
		{
			all_intact = 0;
			break;
		}
	}
	CHECK(all_intact, "all blocks intact after re-allocation");

	for (int i = 0; i < NFRAG; i++)
		ft_free(ptrs[i]);
}

/* ----- 15. Alignment check ----- */
static void test_alignment(void)
{
	SECTION("15. POINTER ALIGNMENT");
	int all_aligned = 1;

	size_t sizes[] = {1, 7, 8, 15, 16, 31, 32, 63, 64, 100, 256, 500, 992, 2000, 8000};
	int n = sizeof(sizes) / sizeof(sizes[0]);
	void *ptrs[15];

	for (int i = 0; i < n; i++)
	{
		ptrs[i] = ft_malloc(sizes[i]);
		if (ptrs[i])
		{
			/* Check 8-byte alignment (minimum for most architectures) */
			if ((uintptr_t)ptrs[i] % 8 != 0)
			{
				all_aligned = 0;
				printf(RED "    ptr %p (size=%zu) NOT 8-byte aligned" RESET "\n", ptrs[i], sizes[i]);
			}
		}
	}
	CHECK(all_aligned, "all pointers are 8-byte aligned");

	/* Check 16-byte alignment (stricter, common on 64-bit) */
	int all_16_aligned = 1;
	for (int i = 0; i < n; i++)
	{
		if (ptrs[i] && (uintptr_t)ptrs[i] % 16 != 0)
		{
			all_16_aligned = 0;
			printf(YELLOW "    ptr %p (size=%zu) NOT 16-byte aligned" RESET "\n", ptrs[i], sizes[i]);
		}
	}
	if (all_16_aligned)
		printf(GREEN "  [INFO] " RESET "all pointers are also 16-byte aligned\n");
	else
		printf(YELLOW "  [WARN] " RESET "some pointers are not 16-byte aligned\n");

	for (int i = 0; i < n; i++)
		ft_free(ptrs[i]);
}

/* ----- 16. Realloc chain ----- */
static void test_realloc_chain(void)
{
	SECTION("16. REALLOC CHAIN (progressive growth)");

	void *p = ft_malloc(1);
	CHECK(p != NULL, "initial malloc(1)");
	*(char *)p = 'X';

	size_t sizes[] = {5, 10, 50, 100, 200, 500, 900, 1500, 3000, 10000};
	int n = sizeof(sizes) / sizeof(sizes[0]);
	int ok = 1;

	for (int i = 0; i < n; i++)
	{
		void *p2 = ft_realloc(p, sizes[i]);
		if (!p2)
		{
			ok = 0;
			break;
		}
		CHECK(*(char *)p2 == 'X', "realloc chain: first byte preserved");
		fill_pattern(p2, sizes[i], (unsigned char)(i + 1));
		*(char *)p2 = 'X'; /* restore marker for next iteration */
		p = p2;
	}
	CHECK(ok, "realloc chain completed successfully");
	ft_free(p);
}

/* ----- 17. Alloc/free interleaved ----- */
static void test_interleaved(void)
{
	SECTION("17. INTERLEAVED ALLOC/FREE");
#define NINTER 3000
	void *ptrs[NINTER];
	memset(ptrs, 0, sizeof(ptrs));
	int ok = 1;

	/* Alloc some, free some, alloc more, etc. */
	for (int round = 0; round < 10; round++)
	{
		int base = round * (NINTER / 10);
		/* Allocate a batch */
		for (int i = base; i < base + NINTER / 10; i++)
		{
			ptrs[i] = ft_malloc((i % 100) + 1);
			if (!ptrs[i])
			{
				ok = 0;
				break;
			}
			fill_pattern(ptrs[i], (i % 100) + 1, (unsigned char)i);
		}
		/* Free half of the previous batch */
		if (round > 0)
		{
			int pbase = (round - 1) * (NINTER / 10);
			for (int i = pbase; i < pbase + NINTER / 20; i++)
			{
				ft_free(ptrs[i]);
				ptrs[i] = NULL;
			}
		}
	}
	CHECK(ok, "interleaved alloc/free: all allocations succeeded");

	/* Free everything remaining */
	for (int i = 0; i < NINTER; i++)
	{
		if (ptrs[i])
			ft_free(ptrs[i]);
	}
	CHECK(1, "interleaved alloc/free: cleanup done");
}

/* ----- 18. Edge case: very large allocation ----- */
static void test_very_large(void)
{
	SECTION("18. VERY LARGE ALLOCATION");

	void *p = ft_malloc(100 * 1024 * 1024); /* 100 MB */
	if (p)
	{
		CHECK(1, "malloc(100MB) succeeded");
		memset(p, 0x42, 100 * 1024 * 1024);
		CHECK(((unsigned char *)p)[50 * 1024 * 1024] == 0x42, "100MB write/read ok");
		ft_free(p);
	}
	else
		printf(YELLOW "  [SKIP] " RESET "malloc(100MB) returned NULL (ok if low memory)\n");

	/* SIZE_MAX should fail gracefully */
	p = ft_malloc((size_t)-1);
	CHECK(p == NULL, "malloc(SIZE_MAX) returns NULL");

	p = ft_malloc((size_t)-2);
	CHECK(p == NULL, "malloc(SIZE_MAX - 1) returns NULL");
}

/* ----- 19. Boundary sizes (around bucket transitions) ----- */
static void test_boundary_sizes(void)
{
	SECTION("19. BOUNDARY SIZES (bucket transitions)");

	/* Test sizes around bucket boundaries: 32, 64, 128, 256, 512, 1024 */
	/* Usable = bucket - sizeof(t_block) = bucket - 16 */
	size_t boundary_sizes[] = {
		/* Around 16 usable (bucket 32) */
		1, 15, 16, 17,
		/* Around 48 usable (bucket 64) */
		47, 48, 49,
		/* Around 112 usable (bucket 128) */
		111, 112, 113,
		/* Around 240 usable (bucket 256) */
		239, 240, 241,
		/* Around 496 usable (bucket 512) */
		495, 496, 497,
		/* Around 992 usable (bucket 1024) */
		991, 992, 993,
		/* Just above 1024 (triggers big alloc) */
		1008, 1024, 1025};
	int n = sizeof(boundary_sizes) / sizeof(boundary_sizes[0]);
	int ok = 1;

	for (int i = 0; i < n; i++)
	{
		void *p = ft_malloc(boundary_sizes[i]);
		char msg[128];
		snprintf(msg, sizeof(msg), "boundary malloc(%zu)", boundary_sizes[i]);
		if (p)
		{
			fill_pattern(p, boundary_sizes[i], (unsigned char)(i * 13));
			if (!verify_pattern(p, boundary_sizes[i], (unsigned char)(i * 13)))
			{
				ok = 0;
				printf(RED "    data corruption at size=%zu" RESET "\n", boundary_sizes[i]);
			}
			CHECK(1, msg);
		}
		else
		{
			CHECK(0, msg);
			ok = 0;
		}
		ft_free(p);
	}
	CHECK(ok, "all boundary sizes: data integrity OK");
}

/* ----- 20. Rapid alloc/free same size ----- */
static void test_rapid_same_size(void)
{
	SECTION("20. RAPID ALLOC/FREE SAME SIZE (recycling)");

	int ok = 1;
	for (int i = 0; i < 10000; i++)
	{
		void *p = ft_malloc(42);
		if (!p)
		{
			ok = 0;
			break;
		}
		memset(p, 0xFF, 42);
		ft_free(p);
	}
	CHECK(ok, "10000 x malloc(42)/free: no failure");
}

/* ----- 21. Alloc all then free all ----- */
static void test_alloc_all_free_all(void)
{
	SECTION("21. ALLOC ALL THEN FREE ALL");
#define NBATCH 3000
	void *ptrs[NBATCH];
	int ok = 1;

	for (int i = 0; i < NBATCH; i++)
	{
		ptrs[i] = ft_malloc(64);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
	}
	CHECK(ok, "3000 x malloc(64) all succeeded");

	for (int i = 0; i < NBATCH; i++)
		ft_free(ptrs[i]);
	CHECK(1, "3000 x free completed");

	/* Reallocate to test reuse */
	ok = 1;
	for (int i = 0; i < NBATCH; i++)
	{
		ptrs[i] = ft_malloc(64);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
	}
	CHECK(ok, "3000 x malloc(64) after free-all: reuse works");

	for (int i = 0; i < NBATCH; i++)
		ft_free(ptrs[i]);
}

/* ----- 22. Thread safety test ----- */
#define NTHREADS 8
#define ALLOCS_PER_THREAD 500

typedef struct s_thread_data
{
	int thread_id;
	int success;
} t_thread_data;

static void *thread_worker(void *arg)
{
	t_thread_data *data = (t_thread_data *)arg;
	void *ptrs[ALLOCS_PER_THREAD];
	data->success = 1;

	for (int i = 0; i < ALLOCS_PER_THREAD; i++)
	{
		size_t sz = ((data->thread_id * 7 + i * 13) % 200) + 1;
		ptrs[i] = ft_malloc(sz);
		if (!ptrs[i])
		{
			data->success = 0;
			/* Free what we have */
			for (int j = 0; j < i; j++)
				ft_free(ptrs[j]);
			return NULL;
		}
		fill_pattern(ptrs[i], sz, (unsigned char)(data->thread_id + i));
	}

	/* Verify */
	for (int i = 0; i < ALLOCS_PER_THREAD; i++)
	{
		size_t sz = ((data->thread_id * 7 + i * 13) % 200) + 1;
		if (!verify_pattern(ptrs[i], sz, (unsigned char)(data->thread_id + i)))
		{
			data->success = 0;
			break;
		}
	}

	/* Free all */
	for (int i = 0; i < ALLOCS_PER_THREAD; i++)
		ft_free(ptrs[i]);

	return NULL;
}

static void test_threads(void)
{
	SECTION("22. THREAD SAFETY");
	pthread_t threads[NTHREADS];
	t_thread_data thread_data[NTHREADS];
	int all_ok = 1;

	for (int i = 0; i < NTHREADS; i++)
	{
		thread_data[i].thread_id = i;
		thread_data[i].success = 0;
		pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
	}

	for (int i = 0; i < NTHREADS; i++)
	{
		pthread_join(threads[i], NULL);
		if (!thread_data[i].success)
		{
			all_ok = 0;
			printf(RED "    thread %d failed" RESET "\n", i);
		}
	}
	CHECK(all_ok, "8 threads x 500 allocs: thread safety OK");
}

/* ----- 23. Thread stress: concurrent alloc/free ----- */
static void *thread_stress_worker(void *arg)
{
	int id = *(int *)arg;
	(void)id;

	for (int round = 0; round < 100; round++)
	{
		void *p1 = ft_malloc(16);
		void *p2 = ft_malloc(100);
		void *p3 = ft_malloc(500);
		void *p4 = ft_malloc(2000);
		if (p1)
		{
			memset(p1, 0xAA, 16);
			ft_free(p1);
		}
		if (p2)
		{
			memset(p2, 0xBB, 100);
			ft_free(p2);
		}
		if (p3)
		{
			memset(p3, 0xCC, 500);
			ft_free(p3);
		}
		if (p4)
		{
			memset(p4, 0xDD, 2000);
			ft_free(p4);
		}
	}
	return NULL;
}

static void test_thread_stress(void)
{
	SECTION("23. THREAD STRESS (concurrent alloc+free)");
	pthread_t threads[NTHREADS];
	int ids[NTHREADS];

	for (int i = 0; i < NTHREADS; i++)
	{
		ids[i] = i;
		pthread_create(&threads[i], NULL, thread_stress_worker, &ids[i]);
	}
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
	CHECK(1, "8 threads x 400 alloc/free: no crash");
}

/* ----- 24. Realloc with NULL and 0 edge cases ----- */
static void test_realloc_edge(void)
{
	SECTION("24. REALLOC EDGE CASES");

	/* realloc(NULL, 0) */
	void *p = ft_realloc(NULL, 0);
	CHECK(p == NULL, "realloc(NULL, 0) returns NULL");

	/* realloc(valid, same_size) */
	p = ft_malloc(50);
	fill_pattern(p, 50, 0x42);
	void *p2 = ft_realloc(p, 50);
	CHECK(p2 != NULL, "realloc(ptr, same_size) returns non-NULL");
	CHECK(verify_pattern(p2, 50, 0x42), "realloc(ptr, same_size) data preserved");
	ft_free(p2);

	/* realloc with size 1 */
	p = ft_malloc(500);
	fill_pattern(p, 1, 0xEE);
	p2 = ft_realloc(p, 1);
	CHECK(p2 != NULL, "realloc(ptr, 1) returns non-NULL");
	CHECK(verify_pattern(p2, 1, 0xEE), "realloc(ptr, 1) first byte preserved");
	ft_free(p2);
}

/* ----- 25. Many big blocks ----- */
static void test_many_big_blocks(void)
{
	SECTION("25. MANY BIG BLOCKS (mmap stress)");
#define NBIG 100
	void *ptrs[NBIG];
	int ok = 1;

	for (int i = 0; i < NBIG; i++)
	{
		ptrs[i] = ft_malloc(4096 + i * 100);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
		fill_pattern(ptrs[i], 4096 + i * 100, (unsigned char)i);
	}
	CHECK(ok, "100 big block allocations");

	int intact = 1;
	for (int i = 0; i < NBIG && ok; i++)
	{
		if (!verify_pattern(ptrs[i], 4096 + i * 100, (unsigned char)i))
		{
			intact = 0;
			printf(RED "    big block %d corrupted" RESET "\n", i);
		}
	}
	CHECK(intact, "100 big blocks data integrity");

	for (int i = 0; i < NBIG; i++)
		ft_free(ptrs[i]);
	CHECK(1, "100 big blocks freed");
}

/* ----- 26. show_alloc_mem does not crash ----- */
static void test_show_alloc(void)
{
	SECTION("26. SHOW_ALLOC_MEM");

	/* With active allocations */
	void *p1 = ft_malloc(10);
	void *p2 = ft_malloc(500);
	void *p3 = ft_malloc(5000);

	/* Suppress output: we only care about not crashing */
	int saved_stdout = dup(STDOUT_FILENO);
	int devnull = open("/dev/null", O_WRONLY);
	dup2(devnull, STDOUT_FILENO);
	close(devnull);

	EXPECT_NO_CRASH(show_alloc_mem());

	ft_free(p1);
	ft_free(p2);
	ft_free(p3);

	EXPECT_NO_CRASH(show_alloc_mem());

	/* Restore stdout */
	dup2(saved_stdout, STDOUT_FILENO);
	close(saved_stdout);

	CHECK(1, "show_alloc_mem() does not crash with active allocs");
	CHECK(1, "show_alloc_mem() does not crash after freeing all");
}

/* ----- 27. Alloc 1 byte, write string ----- */
static void test_single_byte(void)
{
	SECTION("27. SINGLE BYTE ALLOCATION");

	char *p = ft_malloc(1);
	CHECK(p != NULL, "malloc(1) returns non-NULL");
	if (p)
	{
		*p = 'Z';
		CHECK(*p == 'Z', "single byte write/read OK");
		ft_free(p);
	}
}

/* ----- 28. Defragmentation test ----- */
static void test_defragmentation(void)
{
	SECTION("28. DEFRAGMENTATION");
#define NDEFRAG 2000
	void *ptrs[NDEFRAG];
	int ok = 1;

	/* Allocate many blocks of same size */
	for (int i = 0; i < NDEFRAG; i++)
	{
		ptrs[i] = ft_malloc(20);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
	}
	CHECK(ok, "2000 x malloc(20) for defrag test");

	/* Free every other one */
	for (int i = 0; i < NDEFRAG; i += 2)
	{
		ft_free(ptrs[i]);
		ptrs[i] = NULL;
	}

	/* Free the rest */
	for (int i = 1; i < NDEFRAG; i += 2)
	{
		ft_free(ptrs[i]);
		ptrs[i] = NULL;
	}

	/* Reallocate: should reuse merged blocks */
	ok = 1;
	for (int i = 0; i < NDEFRAG; i++)
	{
		ptrs[i] = ft_malloc(20);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
		fill_pattern(ptrs[i], 20, (unsigned char)i);
	}
	CHECK(ok, "2000 x malloc(20) after defrag: reuse OK");

	for (int i = 0; i < NDEFRAG; i++)
		ft_free(ptrs[i]);
}

/* ----- 29. Realloc preserves content across bucket changes ----- */
static void test_realloc_cross_bucket(void)
{
	SECTION("29. REALLOC ACROSS BUCKETS");

	struct
	{
		size_t from;
		size_t to;
	} cases[] = {
		{1, 50},	  /* 32 -> 64 bucket */
		{10, 120},	  /* 32 -> 128 bucket */
		{50, 300},	  /* 64 -> 512 bucket */
		{100, 1000},  /* 128 -> 1024 bucket */
		{200, 2000},  /* 256 -> big */
		{500, 5000},  /* 512 -> big */
		{900, 50000}, /* 1024 -> big */
	};
	int n = sizeof(cases) / sizeof(cases[0]);
	int ok = 1;

	for (int i = 0; i < n; i++)
	{
		void *p = ft_malloc(cases[i].from);
		if (!p)
		{
			ok = 0;
			continue;
		}
		fill_pattern(p, cases[i].from, (unsigned char)(i + 42));

		void *p2 = ft_realloc(p, cases[i].to);
		if (!p2)
		{
			ok = 0;
			continue;
		}

		char msg[128];
		snprintf(msg, sizeof(msg), "realloc(%zu -> %zu) data preserved", cases[i].from, cases[i].to);
		CHECK(verify_pattern(p2, cases[i].from, (unsigned char)(i + 42)), msg);
		ft_free(p2);
	}
	(void)ok;
	CHECK(1, "all cross-bucket reallocs completed");
}

/* ----- 30. Malloc after heavy fragmentation ----- */
static void test_post_fragmentation_alloc(void)
{
	SECTION("30. ALLOCATION AFTER HEAVY FRAGMENTATION");
#define NHEAVY 4000
	void *ptrs[NHEAVY];
	int ok = 1;

	/* Allocate many */
	for (int i = 0; i < NHEAVY; i++)
	{
		ptrs[i] = ft_malloc((i % 5 + 1) * 10);
		if (!ptrs[i])
		{
			ok = 0;
			break;
		}
	}

	/* Free in a pattern: free 3 out of every 4 */
	for (int i = 0; i < NHEAVY; i++)
	{
		if (i % 4 != 0)
		{
			ft_free(ptrs[i]);
			ptrs[i] = NULL;
		}
	}

	/* Allocate in holes */
	int new_ok = 1;
	void *new_ptrs[1000];
	for (int i = 0; i < 1000; i++)
	{
		new_ptrs[i] = ft_malloc(10);
		if (!new_ptrs[i])
		{
			new_ok = 0;
			break;
		}
		fill_pattern(new_ptrs[i], 10, (unsigned char)i);
	}
	CHECK(new_ok, "1000 new allocs after heavy fragmentation");

	/* Cleanup */
	for (int i = 0; i < NHEAVY; i++)
		if (ptrs[i])
			ft_free(ptrs[i]);
	for (int i = 0; i < 1000; i++)
		if (new_ptrs[i])
			ft_free(new_ptrs[i]);
}

/* ----- 31. Realloc big to small ----- */
static void test_realloc_big_to_small(void)
{
	SECTION("31. REALLOC BIG -> SMALL");

	void *p = ft_malloc(10000);
	CHECK(p != NULL, "malloc(10000)");
	fill_pattern(p, 10, 0xAB);

	void *p2 = ft_realloc(p, 10);
	CHECK(p2 != NULL, "realloc(10000 -> 10)");
	CHECK(verify_pattern(p2, 10, 0xAB), "data preserved (first 10 bytes)");
	ft_free(p2);
}

/* ----- 32. Consecutive realloc shrink ----- */
static void test_realloc_shrink_chain(void)
{
	SECTION("32. REALLOC SHRINK CHAIN");

	void *p = ft_malloc(1000);
	CHECK(p != NULL, "malloc(1000)");
	fill_pattern(p, 1000, 0x77);

	size_t sizes[] = {900, 500, 200, 100, 50, 10, 1};
	int n = sizeof(sizes) / sizeof(sizes[0]);

	for (int i = 0; i < n; i++)
	{
		void *p2 = ft_realloc(p, sizes[i]);
		CHECK(p2 != NULL, "shrink realloc step");
		CHECK(verify_pattern(p2, sizes[i], 0x77), "data preserved after shrink");
		p = p2;
	}
	ft_free(p);
}

/* ----- 33. String operations ----- */
static void test_string_operations(void)
{
	SECTION("33. STRING OPERATIONS (real-world usage)");

	/* Simulate strdup */
	const char *original = "Hello, World! This is a malloc test string.";
	size_t len = strlen(original) + 1;
	char *dup = ft_malloc(len);
	CHECK(dup != NULL, "malloc for string dup");
	if (dup)
	{
		memcpy(dup, original, len);
		CHECK(strcmp(dup, original) == 0, "string copy correct");
		ft_free(dup);
	}

	/* Simulate string concatenation */
	const char *s1 = "Hello ";
	const char *s2 = "World!";
	size_t l1 = strlen(s1), l2 = strlen(s2);
	char *concat = ft_malloc(l1 + l2 + 1);
	CHECK(concat != NULL, "malloc for string concat");
	if (concat)
	{
		memcpy(concat, s1, l1);
		memcpy(concat + l1, s2, l2 + 1);
		CHECK(strcmp(concat, "Hello World!") == 0, "string concatenation correct");
		ft_free(concat);
	}

/* Array of strings */
#define NSTRINGS 100
	char *strings[NSTRINGS];
	int ok = 1;
	for (int i = 0; i < NSTRINGS; i++)
	{
		strings[i] = ft_malloc(32);
		if (!strings[i])
		{
			ok = 0;
			break;
		}
		snprintf(strings[i], 32, "string_%04d", i);
	}
	CHECK(ok, "100 string allocations");

	int str_ok = 1;
	for (int i = 0; i < NSTRINGS && ok; i++)
	{
		char expected[32];
		snprintf(expected, 32, "string_%04d", i);
		if (strcmp(strings[i], expected) != 0)
		{
			str_ok = 0;
			printf(RED "    string[%d] = '%s', expected '%s'" RESET "\n", i, strings[i], expected);
		}
	}
	CHECK(str_ok, "100 strings integrity check");

	for (int i = 0; i < NSTRINGS; i++)
		if (strings[i])
			ft_free(strings[i]);
}

/* ----- 34. Struct allocation ----- */
static void test_struct_alloc(void)
{
	SECTION("34. STRUCT ALLOCATION (real-world)");

	typedef struct
	{
		int id;
		double value;
		char name[32];
	} Item;

#define NITEMS 500
	Item **items = ft_malloc(sizeof(Item *) * NITEMS);
	CHECK(items != NULL, "malloc for pointer array");

	int ok = 1;
	if (items)
	{
		for (int i = 0; i < NITEMS; i++)
		{
			items[i] = ft_malloc(sizeof(Item));
			if (!items[i])
			{
				ok = 0;
				break;
			}
			items[i]->id = i;
			items[i]->value = i * 3.14;
			snprintf(items[i]->name, 32, "item_%d", i);
		}
		CHECK(ok, "500 struct allocations");

		int verify_ok = 1;
		for (int i = 0; i < NITEMS && ok; i++)
		{
			if (items[i]->id != i || items[i]->value != i * 3.14)
			{
				verify_ok = 0;
				printf(RED "    item[%d] corrupted" RESET "\n", i);
			}
		}
		CHECK(verify_ok, "500 struct integrity check");

		for (int i = 0; i < NITEMS; i++)
			if (items[i])
				ft_free(items[i]);
		ft_free(items);
	}
}

/* ----- 35. Linked list simulation ----- */
static void test_linked_list(void)
{
	SECTION("35. LINKED LIST SIMULATION");

	typedef struct s_node
	{
		int value;
		struct s_node *next;
	} t_node;

	t_node *head = NULL;
	int ok = 1;
#define NLIST 1000

	/* Build list */
	for (int i = 0; i < NLIST; i++)
	{
		t_node *node = ft_malloc(sizeof(t_node));
		if (!node)
		{
			ok = 0;
			break;
		}
		node->value = i;
		node->next = head;
		head = node;
	}
	CHECK(ok, "1000-node linked list created");

	/* Verify values (should be in reverse order) */
	t_node *curr = head;
	int verify_ok = 1;
	int expected = NLIST - 1;
	while (curr)
	{
		if (curr->value != expected)
		{
			verify_ok = 0;
			break;
		}
		expected--;
		curr = curr->next;
	}
	CHECK(verify_ok && expected == -1, "linked list values correct");

	/* Free list */
	while (head)
	{
		t_node *tmp = head;
		head = head->next;
		ft_free(tmp);
	}
	CHECK(1, "linked list freed");
}

/* ----- 36. Random-ish pattern ----- */
static void test_random_pattern(void)
{
	SECTION("36. PSEUDO-RANDOM ALLOC/FREE PATTERN");

#define NRAND 2000
	void *ptrs[NRAND];
	memset(ptrs, 0, sizeof(ptrs));
	int ok = 1;
	unsigned int seed = 42;

	for (int iter = 0; iter < 10000; iter++)
	{
		seed = seed * 1103515245 + 12345; /* LCG */
		int idx = (seed >> 16) % NRAND;
		if (ptrs[idx])
		{
			ft_free(ptrs[idx]);
			ptrs[idx] = NULL;
		}
		else
		{
			size_t sz = ((seed >> 8) % 500) + 1;
			ptrs[idx] = ft_malloc(sz);
			if (!ptrs[idx])
			{
				ok = 0;
				break;
			}
			memset(ptrs[idx], 0x42, sz);
		}
	}
	CHECK(ok, "10000 pseudo-random operations: no failure");

	for (int i = 0; i < NRAND; i++)
		if (ptrs[i])
			ft_free(ptrs[i]);
	CHECK(1, "random pattern cleanup done");
}

/* ----- 37. Zero-fill check ----- */
static void test_zero_fill(void)
{
	SECTION("37. MEMORY CONTENT (not guaranteed zero)");

	/* Just verify we can write and read back */
	void *p = ft_malloc(1024);
	CHECK(p != NULL, "malloc(1024)");
	if (p)
	{
		memset(p, 0, 1024);
		int all_zero = 1;
		for (int i = 0; i < 1024; i++)
		{
			if (((unsigned char *)p)[i] != 0)
			{
				all_zero = 0;
				break;
			}
		}
		CHECK(all_zero, "memset(0) + verify: ok");
		ft_free(p);
	}
}

/* ----- 38. Torture: alternating sizes ----- */
static void test_alternating_sizes(void)
{
	SECTION("38. ALTERNATING SIZES TORTURE");

	int ok = 1;
	void *p = NULL;
	size_t prev_sz = 0;

	for (int i = 0; i < 5000; i++)
	{
		size_t sz = (i % 2 == 0) ? 10 : 900;
		void *np = ft_malloc(sz);
		if (!np)
		{
			ok = 0;
			break;
		}
		fill_pattern(np, sz, (unsigned char)i);
		if (p)
			ft_free(p);
		p = np;
		prev_sz = sz;
	}
	if (p)
		ft_free(p);
	(void)prev_sz;
	CHECK(ok, "5000 alternating small/large allocs: OK");
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/*                                MAIN                                       */
/* ═══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
	/* Setup crash handlers */
	signal(SIGSEGV, crash_handler);
	signal(SIGBUS, crash_handler);
	signal(SIGABRT, crash_handler);

	printf(BOLD "\n╔══════════════════════════════════════════════════════════╗\n");
	printf("║          FT_MALLOC EXHAUSTIVE TESTER                     ║\n");
	printf("╚══════════════════════════════════════════════════════════╝\n" RESET);

	test_basic_malloc();			 /* 1 */
	test_malloc_zero();				 /* 2 */
	test_big_alloc();				 /* 3 */
	test_free_null();				 /* 4 */
	test_double_free();				 /* 5 */
	test_write_read_integrity();	 /* 6 */
	test_no_overlap();				 /* 7 */
	test_realloc_basic();			 /* 8 */
	test_realloc_grow();			 /* 9 */
	test_realloc_shrink();			 /* 10 */
	test_stress_small();			 /* 11 */
	test_stress_mixed();			 /* 12 */
	test_free_reverse();			 /* 13 */
	test_fragmentation();			 /* 14 */
	test_alignment();				 /* 15 */
	test_realloc_chain();			 /* 16 */
	test_interleaved();				 /* 17 */
	test_very_large();				 /* 18 */
	test_boundary_sizes();			 /* 19 */
	test_rapid_same_size();			 /* 20 */
	test_alloc_all_free_all();		 /* 21 */
	test_threads();					 /* 22 */
	test_thread_stress();			 /* 23 */
	test_realloc_edge();			 /* 24 */
	test_many_big_blocks();			 /* 25 */
	test_show_alloc();				 /* 26 */
	test_single_byte();				 /* 27 */
	test_defragmentation();			 /* 28 */
	test_realloc_cross_bucket();	 /* 29 */
	test_post_fragmentation_alloc(); /* 30 */
	test_realloc_big_to_small();	 /* 31 */
	test_realloc_shrink_chain();	 /* 32 */
	test_string_operations();		 /* 33 */
	test_struct_alloc();			 /* 34 */
	test_linked_list();				 /* 35 */
	test_random_pattern();			 /* 36 */
	test_zero_fill();				 /* 37 */
	test_alternating_sizes();		 /* 38 */

	/* ===== SUMMARY ===== */
	printf(BOLD "\n╔══════════════════════════════════════════════════════════╗\n");
	printf("║                      RESULTS                             ║\n");
	printf("╠══════════════════════════════════════════════════════════╣\n");
	printf("║  Total: %-4d  |  " GREEN "Pass: %-4d" RESET BOLD "  |  " RED "Fail: %-4d" RESET BOLD "          ║\n",
		   g_total, g_pass, g_fail);
	printf("╚══════════════════════════════════════════════════════════╝\n" RESET);

	if (g_fail == 0)
		printf(GREEN BOLD "\n🎉  ALL TESTS PASSED!\n\n" RESET);
	else
		printf(RED BOLD "\n💥  %d TEST(S) FAILED!\n\n" RESET, g_fail);

	return (g_fail > 0);
}
