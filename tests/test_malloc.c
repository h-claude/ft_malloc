#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_alloc_mem(void);

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, name) check((cond), (name))

static void check(int cond, const char *name)
{
	if (cond)
		g_pass++;
	else
		g_fail++;
	printf("  [%s] %s\n", cond ? "PASS" : "FAIL", name);
}

// Hides the pointer from the compiler so it doesn't warn about (or optimize away) bad frees
static void *opaque(void *ptr)
{
	void *volatile p = ptr;

	return (p);
}

static int is_filled(const char *ptr, char c, size_t n)
{
	for (size_t i = 0; i < n; i++)
		if (ptr[i] != c)
			return (0);
	return (1);
}

static void test_basic(void)
{
	static const size_t sizes[] = {1, 16, 42, 48, 100, 112, 200, 496, 1000, 1024, 1025, 4096, 100000};
	size_t n = sizeof(sizes) / sizeof(sizes[0]);
	char *ptrs[13];
	int ok_align = 1;
	int ok_data = 1;

	puts("\n== malloc: TINY / SMALL / LARGE ==");
	for (size_t i = 0; i < n; i++)
	{
		ptrs[i] = malloc(sizes[i]);
		if (!ptrs[i])
		{
			CHECK(0, "malloc returns non-NULL");
			return;
		}
		if ((uintptr_t)ptrs[i] % 16)
			ok_align = 0;
		memset(ptrs[i], (char)('a' + i), sizes[i]);
	}
	for (size_t i = 0; i < n; i++)
		if (!is_filled(ptrs[i], (char)('a' + i), sizes[i]))
			ok_data = 0;
	CHECK(1, "malloc returns non-NULL for every size");
	CHECK(ok_align, "every pointer is 16-byte aligned");
	CHECK(ok_data, "blocks don't overlap (data intact after filling all)");
	for (size_t i = 0; i < n; i++)
		free(ptrs[i]);
}

static void test_malloc_edge(void)
{
	volatile size_t zero = 0;
	volatile size_t huge = SIZE_MAX;

	puts("\n== malloc: edge cases ==");
	CHECK(malloc(zero) == NULL, "malloc(0) returns NULL");
	CHECK(malloc(huge) == NULL, "malloc(SIZE_MAX) returns NULL");
	CHECK(malloc(huge - 8) == NULL, "malloc(SIZE_MAX - 8) returns NULL (header overflow)");
}

static void test_free_edge(void)
{
	char stack_var[32];
	char *p;

	puts("\n== free: invalid pointers (must not crash) ==");
	free(NULL);
	CHECK(1, "free(NULL)");
	free(opaque((void *)0x1234));
	CHECK(1, "free(random address)");
	free(opaque(stack_var));
	CHECK(1, "free(stack address)");
	p = malloc(100);
	free(opaque(p + 8));
	CHECK(1, "free(pointer in the middle of a TINY block)");
	p[0] = 'x';
	CHECK(p[0] == 'x', "block still usable after the bad free");
	free(p);
	free(opaque(p));
	CHECK(1, "double free TINY");
	p = malloc(50000);
	free(opaque(p + 100));
	CHECK(1, "free(pointer in the middle of a LARGE block)");
	free(p);
	CHECK(1, "free LARGE");
}

static void test_reuse(void)
{
	char *a;
	char *b;

	puts("\n== free + malloc: memory reuse ==");
	a = malloc(42);
	free(a);
	b = malloc(42);
	CHECK(a == b, "a freed block is reused by the next malloc of the same class");
	free(b);
}

static void test_realloc(void)
{
	volatile size_t huge = SIZE_MAX - 8;
	char *p;
	char *q;

	puts("\n== realloc ==");
	p = realloc(NULL, 10);
	CHECK(p != NULL, "realloc(NULL, 10) behaves like malloc");
	strcpy(p, "hello");
	q = realloc(p, 8);
	CHECK(q == p, "shrinking keeps the same pointer");
	p = realloc(q, 300);
	CHECK(p && strcmp(p, "hello") == 0, "TINY -> SMALL keeps the data");
	p = realloc(p, 5000);
	CHECK(p && strcmp(p, "hello") == 0, "SMALL -> LARGE keeps the data");
	p = realloc(p, 100000);
	CHECK(p && strcmp(p, "hello") == 0, "LARGE -> bigger LARGE keeps the data");
	free(p);

	p = malloc(2001);
	memset(p, 'x', 2001);
	p[2000] = 'Z';
	p = realloc(p, 10000);
	CHECK(p && p[2000] == 'Z', "odd-sized LARGE keeps its last byte after realloc");
	free(p);

	p = malloc(100);
	strcpy(p, "keep me");
	CHECK(realloc(p, huge) == NULL, "realloc(p, huge) returns NULL");
	CHECK(strcmp(p, "keep me") == 0, "original block intact after a failed realloc");
	CHECK(realloc(opaque(p + 32), 4000) == NULL, "realloc(pointer in the middle of a block) returns NULL");
	CHECK(realloc(opaque((void *)0x1234), 10) == NULL, "realloc(random address) returns NULL");
	CHECK(realloc(p, 0) == NULL, "realloc(p, 0) frees and returns NULL");
}

static void test_stress(void)
{
	static char *ptrs[10000];
	static size_t sizes[10000];
	int ok = 1;

	puts("\n== stress: 10000 mixed allocations ==");
	for (int round = 0; round < 3; round++)
	{
		for (int i = 0; i < 10000; i++)
		{
			sizes[i] = (size_t)(i * 37 + round) % 3000 + 1;
			ptrs[i] = malloc(sizes[i]);
			if (!ptrs[i])
				ok = 0;
			else
				memset(ptrs[i], (char)i, sizes[i]);
		}
		for (int i = 0; i < 10000; i += 2)
			free(ptrs[i]);
		for (int i = 1; i < 10000; i += 2)
		{
			if (!is_filled(ptrs[i], (char)i, sizes[i]))
				ok = 0;
			ptrs[i] = realloc(ptrs[i], (size_t)(i * 13) % 3000 + 1);
		}
		for (int i = 1; i < 10000; i += 2)
			free(ptrs[i]);
	}
	CHECK(ok, "no NULL, no corrupted data over 3 rounds");
}

static void *thread_routine(void *arg)
{
	long id = (long)arg;

	for (int r = 0; r < 20000; r++)
	{
		size_t n = (size_t)(r * 31 + id) % 3000 + 1;
		char *p = malloc(n);

		if (!p)
			return ((void *)1);
		memset(p, (char)id, n);
		p = realloc(p, n * 2);
		if (!p || !is_filled(p, (char)id, n))
			return ((void *)1);
		free(p);
	}
	return (NULL);
}

static void test_threads(void)
{
	pthread_t threads[8];
	void *ret;
	int ok = 1;

	puts("\n== threads: 8 x 20000 malloc/realloc/free ==");
	for (long i = 0; i < 8; i++)
		pthread_create(&threads[i], NULL, thread_routine, (void *)i);
	for (int i = 0; i < 8; i++)
	{
		pthread_join(threads[i], &ret);
		if (ret)
			ok = 0;
	}
	CHECK(ok, "no corruption between threads");
}

static void test_show_alloc_mem(void)
{
	char *p[6];

	puts("\n== show_alloc_mem (check by eye: increasing addresses) ==");
	p[0] = malloc(42);
	p[1] = malloc(84);
	p[2] = malloc(300);
	p[3] = malloc(900);
	p[4] = malloc(3725);
	p[5] = malloc(48847);
	fflush(stdout);
	show_alloc_mem();
	for (int i = 0; i < 6; i++)
		free(p[i]);
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	test_basic();
	test_malloc_edge();
	test_free_edge();
	test_reuse();
	test_realloc();
	test_stress();
	test_threads();
	test_show_alloc_mem();
	printf("\n== %d passed, %d failed ==\n", g_pass, g_fail);
	return (g_fail != 0);
}
