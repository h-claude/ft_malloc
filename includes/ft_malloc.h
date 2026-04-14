/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 17:57:26 by hclaude           #+#    #+#             */
/*   Updated: 2026/04/14 16:44:13 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
#define FT_MALLOC_H

#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#ifdef MALLOC_DEBUG_BUILD
# include <stdatomic.h>
#endif

#define BLOCKS_32 0	  // In reality its a block of size 16 bytes
#define BLOCKS_64 1	  // In reality its a block of size 48 bytes
#define BLOCKS_128 2  // In reality its a block of size 112 bytes
#define BLOCKS_256 3  // In reality its a block of size 240 bytes
#define BLOCKS_512 4  // In reality its a block of size 496 bytes
#define BLOCKS_1024 5 // In reality its a block of size 992 bytes

#define FLAG_FREE ((size_t)1)
#define IS_FREE(sz) ((sz) & FLAG_FREE)
#define SET_FREE(sz) ((sz) | FLAG_FREE)
#define SET_ALLOC(sz) ((sz) & ~FLAG_FREE)
#define SIZE_VALUE(sz) ((sz) & ~FLAG_FREE)

#define DEFAULT_PAGE_COUNT 128
#define ARENA_SIZE (DEFAULT_PAGE_COUNT * g_data.pagesize + sizeof(t_arena))

#define MIN_BLOCKS_TO_DEFRAG 3

typedef struct s_block
{
	struct s_block *next;
	size_t size;
} t_block;

typedef struct s_free_blocks
{
	t_block *blocks[6];
	int size_blocks[6];
} t_free_blocks;

typedef struct s_allocated_blocks
{
	t_block *blocks[6];
	int size_blocks[6];
} t_allocated_blocks;

typedef struct s_big_blocks
{
	t_block *blocks;
	size_t size_blocks;
} t_big_blocks;

typedef struct s_arena
{
	struct s_arena *next;
	char			_pad[8];
} t_arena;

typedef struct s_data
{
	t_free_blocks free_blocks;
	t_allocated_blocks allocated_blocks;
	t_big_blocks big_blocks;
	t_arena *arena;
	int pagesize;
#ifdef MALLOC_DEBUG_BUILD
	_Atomic size_t dbg_malloc_calls;
	_Atomic size_t dbg_free_calls;
	_Atomic size_t dbg_live_mem;
#endif
} t_data;

extern t_data g_data;
extern pthread_mutex_t g_mutex;

// it allows to export only the public functions (malloc, free, realloc, show_alloc_mem) and keep the rest hidden
#define PUBLIC __attribute__((visibility("default")))

// Shared functions

PUBLIC void *malloc(size_t size) __attribute__((alloc_size(1)));
PUBLIC void free(void *ptr);
PUBLIC void *realloc(void *ptr, size_t size);
PUBLIC void show_alloc_mem(void);

// Private functions

int init_data();
int size_to_size_index(size_t size);
t_block *get_last_block(int size_index, int is_allocated);
int Is_In_ArenA(void *adress);
t_block *remove_block(t_block *block, int is_allocated);
int Is_In_BigBlocks(void *adress);
#ifdef MALLOC_DEBUG_BUILD
void start_debug_thread(void);
#endif

#endif