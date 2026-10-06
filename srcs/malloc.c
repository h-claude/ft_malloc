/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:48:46 by hclaude           #+#    #+#             */
/*   Updated: 2026/10/06 17:51:35 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_data g_data = {0};
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int init_zone(int index)
{
	size_t	block_size;
	size_t	pages;
	size_t	total;
	void	*ptr;
	t_arena	*arena;

	block_size = block_size_for_index(index);
	pages = (sizeof(t_arena) + 100 * block_size + g_data.pagesize - 1) / g_data.pagesize;
	total = pages * g_data.pagesize;
	ptr = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
		return (-1);
	arena = (t_arena *)ptr;
	arena->size = total;
	arena->next = g_data.arena;
	g_data.arena = arena;
	g_data.zone_bump[index] = (char *)ptr + sizeof(t_arena);
	g_data.zone_end[index] = (char *)ptr + total;
	return (0);
}

static t_block *carve_bump(int index_size)
{
	t_block *block;

	if (!g_data.zone_bump[index_size] ||
		(char *)g_data.zone_bump[index_size] + block_size_for_index(index_size) > (char *)g_data.zone_end[index_size])
		return (NULL);
	block = (t_block *)g_data.zone_bump[index_size];
	block->size = SET_ALLOC(block_size_for_index(index_size));
	block->next = g_data.allocated_blocks.blocks[index_size];
	g_data.allocated_blocks.blocks[index_size] = block;
	g_data.allocated_blocks.size_blocks[index_size]++;
	g_data.zone_bump[index_size] = (char *)g_data.zone_bump[index_size] + block_size_for_index(index_size);
	return (block);
}

static void get_more_blocks(int index_size)
{
	t_block *tmp_block;
	t_block *new_block;
	int     tmp_index;

	tmp_index = index_size + 1;
	while (!g_data.free_blocks.size_blocks[index_size] && tmp_index < 6)
	{
		if (!g_data.free_blocks.size_blocks[tmp_index])
			tmp_index++;
		else
		{
			tmp_block = g_data.free_blocks.blocks[tmp_index];
			if (!tmp_block)
				break;
			g_data.free_blocks.blocks[tmp_index] = tmp_block->next;
			size_t block_size = SIZE_VALUE(tmp_block->size);
			if (block_size != block_size_for_index(tmp_index) || block_size < 2 * sizeof(t_block))
			{
				g_data.free_blocks.blocks[tmp_index] = tmp_block;
				break;
			}
			g_data.free_blocks.size_blocks[tmp_index]--;
			size_t half = block_size_for_index(tmp_index - 1);
			new_block = (t_block *)((char *)tmp_block + half);
			tmp_block->size = SET_FREE(half);
			new_block->size = SET_FREE(half);
			tmp_index = size_to_size_index(half);
			tmp_block->next = new_block;
			new_block->next = g_data.free_blocks.blocks[tmp_index];
			g_data.free_blocks.blocks[tmp_index] = tmp_block;
			g_data.free_blocks.size_blocks[tmp_index] += 2;
		}
	}
	if (!g_data.free_blocks.size_blocks[index_size])
		init_zone(index_size);
}

static t_block *pop_free_to_allocated(int index_size)
{
	t_block *block;

	block = g_data.free_blocks.blocks[index_size];
	g_data.free_blocks.blocks[index_size] = block->next;
	block->size = SET_ALLOC(block->size);
	block->next = g_data.allocated_blocks.blocks[index_size];
	g_data.allocated_blocks.blocks[index_size] = block;
	g_data.free_blocks.size_blocks[index_size]--;
	g_data.allocated_blocks.size_blocks[index_size]++;
	return (block);
}

static void *alloc_block(int index_size)
{
	t_block *ret_ptr;

	if (g_data.free_blocks.size_blocks[index_size])
		ret_ptr = pop_free_to_allocated(index_size);
	else
	{
		ret_ptr = carve_bump(index_size);
		if (!ret_ptr)
		{
			get_more_blocks(index_size);
			if (g_data.free_blocks.size_blocks[index_size])
				ret_ptr = pop_free_to_allocated(index_size);
			else
				ret_ptr = carve_bump(index_size);
		}
	}
	if (!ret_ptr)
		return (NULL);
	return ((char *)ret_ptr + sizeof(t_block));
}

static void *alloc_big_block(size_t size)
{
	void *ptr;
	t_block *header;

	ptr = mmap(NULL, size + sizeof(t_block), PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
		return (NULL);
	header = (t_block *)ptr;
	header->size = size;
	header->next = g_data.big_blocks.blocks;
	g_data.big_blocks.blocks = header;
	g_data.big_blocks.size_blocks++;
	return ((char *)header + sizeof(t_block));
}

void *malloc_unlocked(size_t size)
{
	int index_size = 0;
	if (g_data.pagesize == 0)
	{
		if (init_data() == -1)
		{
			return (NULL);
		}
	}
	if (size == 0 || size > SIZE_MAX - sizeof(t_block))
	{
		return (NULL);
	}

	index_size = size_to_size_index(size + sizeof(t_block));

	if (index_size == -1 && size + sizeof(t_block) > 1040)
		return (alloc_big_block(size));
	else if (index_size == -1)
		return (NULL);

	return (alloc_block(index_size));
}

void *malloc(size_t size)
{
	void *ptr;

	pthread_mutex_lock(&g_mutex);
	ptr = malloc_unlocked(size);
	pthread_mutex_unlock(&g_mutex);
	return (ptr);
}
