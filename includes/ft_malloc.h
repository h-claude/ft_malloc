/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 17:57:26 by hclaude           #+#    #+#             */
/*   Updated: 2026/02/02 11:46:47 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
#define FT_MALLOC_H

#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <pthread.h>
#include <stdbool.h>

#define BLOCKS_32 0 // In reality its a block of size 16 bytes
#define BLOCKS_64 1 // In reality its a block of size 48 bytes
#define BLOCKS_128 2 // In reality its a block of size 112 bytes
#define BLOCKS_256 3 // In reality its a block of size 240 bytes
#define BLOCKS_512 4 // In reality its a block of size 496 bytes
#define BLOCKS_1024 5 // In reality its a block of size 992 bytes6 m

#define DEFAULT_PAGE_COUNT 16

typedef struct s_block
{
	struct s_block	*next;
	size_t			size;
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

typedef struct s_data
{
	t_free_blocks free_blocks;
	t_allocated_blocks allocated_blocks;
} t_data;

extern t_data g_data;
extern pthread_mutex_t g_mutex;
extern int g_pagesize;

// Shared functions

void *ft_malloc(size_t size) __attribute__((alloc_size(1)));
void ft_free(void *ptr);
void *ft_realloc(void *ptr, size_t size);
void *show_alloc_mem(void);
void visualize_memory(int detailed);

// Private functions

int init_data();
int size_to_size_index(size_t size);
t_block *get_last_block(int size_index, int is_allocated);

#endif