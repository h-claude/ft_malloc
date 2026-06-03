/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 18:42:03 by hclaude           #+#    #+#             */
/*   Updated: 2026/06/02 17:15:30 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static int same_arena(void *a, void *b)
{
	t_arena *arena;
	void    *start;
	void    *end;

	arena = g_data.arena;
	while (arena)
	{
		start = (char *)arena + sizeof(t_arena);
		end = (char *)arena + arena->size;
		if (a >= start && a < end && b >= start && b < end)
			return (1);
		arena = arena->next;
	}
	return (0);
}

static void free_big_block_unlocked(t_block *header)
{
	t_block *big_curr;
	t_block *big_prev;
	size_t total_size;

	total_size = SIZE_VALUE(header->size) + sizeof(t_block);
	big_curr = g_data.big_blocks.blocks;
	big_prev = NULL;
	while (big_curr && big_curr != header)
	{
		big_prev = big_curr;
		big_curr = big_curr->next;
	}
	if (big_curr)
	{
		if (big_prev)
			big_prev->next = big_curr->next;
		else
			g_data.big_blocks.blocks = big_curr->next;
		g_data.big_blocks.size_blocks--;
	}
	munmap(header, total_size);
}

void free_unlocked(void *ptr)
{
	t_block *header;
	int size_index;
	t_block *curr;
	t_block *prev;
	int in_arena;
	int in_big;

	if (!ptr)
		return;

	header = (t_block *)((char *)ptr - sizeof(t_block));
	in_arena = Is_In_ArenA(header);
	in_big = Is_In_BigBlocks(ptr);
	if (!in_arena && !in_big)
		return;
	if (in_big)
		return (free_big_block_unlocked(header));

	if (header->size == 0 || IS_FREE(header->size))
		return;

	size_t real_size = SIZE_VALUE(header->size);
	size_index = size_to_size_index(real_size);

	if (size_index == -1)
		return;

	curr = g_data.allocated_blocks.blocks[size_index];
	prev = NULL;

	while (curr && curr != header)
	{
		prev = curr;
		curr = curr->next;
	}

	if (curr == NULL)
		return;

	if (prev)
		prev->next = curr->next;
	else
		g_data.allocated_blocks.blocks[size_index] = curr->next;

	g_data.allocated_blocks.size_blocks[size_index]--;

	header->size = SET_FREE(header->size);
	t_block *next_header = (t_block *)((char *)header + SIZE_VALUE(header->size));
	int free_count = g_data.free_blocks.size_blocks[size_index];

	if (free_count > MIN_BLOCKS_TO_DEFRAG && same_arena(header, next_header))
	{
		if (SIZE_VALUE(header->size) == SIZE_VALUE(next_header->size) && IS_FREE(next_header->size) && SIZE_VALUE(header->size) < 512)
		{
			if (remove_block(next_header, 0))
			{
				header->size = SET_FREE(SIZE_VALUE(header->size) * 2);
				size_index = size_to_size_index(SIZE_VALUE(header->size));
			}
		}
	}

	header->next = g_data.free_blocks.blocks[size_index];
	g_data.free_blocks.blocks[size_index] = header;
	g_data.free_blocks.size_blocks[size_index]++;
}

void free(void *ptr)
{
	pthread_mutex_lock(&g_mutex);
	free_unlocked(ptr);
	pthread_mutex_unlock(&g_mutex);
}