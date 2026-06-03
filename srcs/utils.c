/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:49:24 by hclaude           #+#    #+#             */
/*   Updated: 2026/06/03 16:43:12 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

size_t	block_size_for_index(int index)
{
	static const size_t sizes[6] = {32, 64, 128, 256, 512, 1040};

	if (index < 0 || index >= 6)
		return (0);
	return (sizes[index]);
}

int size_to_size_index(size_t size)
{
	static const size_t sizes[] = {32, 64, 128, 256, 512, 1040};

	if (size == 0 || size > 1040)
		return (-1);
	for (int i = 0; i < 6; i++)
		if (size <= sizes[i])
			return (i);
	return (-1);
}

t_block *get_last_block(int size_index, int is_allocated)
{
	if (size_index < 0 || size_index > 5)
		return (NULL);
	if (is_allocated)
	{
		t_block *last_block = g_data.allocated_blocks.blocks[size_index];
		while (last_block && last_block->next)
			last_block = last_block->next;
		return (last_block);
	}
	else
	{
		t_block *last_block = g_data.free_blocks.blocks[size_index];
		while (last_block && last_block->next)
			last_block = last_block->next;
		return (last_block);
	}
}

t_block *get_previous_block(t_block *block, int size_index, int is_allocated)
{
	t_block *current;

	if (is_allocated)
		current = g_data.allocated_blocks.blocks[size_index];
	else
		current = g_data.free_blocks.blocks[size_index];

	if (!current || current == block)
		return (NULL);
	while (current && current->next != block)
		current = current->next;
	return (current);
}

t_block *remove_block(t_block *block, int is_allocated)
{
	t_block *prev;
	t_block *current;
	int size_index;

	if (!block)
		return (NULL);

	size_index = size_to_size_index(SIZE_VALUE(block->size));
	if (is_allocated)
		current = g_data.allocated_blocks.blocks[size_index];
	else
		current = g_data.free_blocks.blocks[size_index];
	prev = NULL;
	while (current && current != block)
	{
		prev = current;
		current = current->next;
	}
	if (!current)
		return (NULL);

	if (prev)
		prev->next = block->next;
	else
	{
		if (is_allocated)
			g_data.allocated_blocks.blocks[size_index] = block->next;
		else
			g_data.free_blocks.blocks[size_index] = block->next;
	}
	if (is_allocated)
		g_data.allocated_blocks.size_blocks[size_index]--;
	else
		g_data.free_blocks.size_blocks[size_index]--;
	block->next = NULL;
	return (block);
}

int init_data()
{
	if (g_data.pagesize == 0)
	{
		g_data.pagesize = getpagesize();
		g_data.arena = NULL;
		g_data.big_blocks.blocks = NULL;
		g_data.big_blocks.size_blocks = 0;
	}
	return (0);
}

/*
	Checks if the given address belongs to any of the arenas.
	Returns 1 if it does, 0 otherwise.
*/
int Is_In_ArenA(void *adress)
{
	t_arena *arena_adress = g_data.arena;

	while (arena_adress)
	{
		void *start = (char *)arena_adress + sizeof(t_arena);
		void *end = (char *)arena_adress + arena_adress->size;
		if (adress >= start && adress < end)
			return (1);
		arena_adress = arena_adress->next;
	}
	return (0);
}

int Is_In_BigBlocks(void *adress)
{
	t_block *current = g_data.big_blocks.blocks;

	while (current)
	{
		void *start = (char *)current + sizeof(t_block);
		void *end = (char *)current + sizeof(t_block) + SIZE_VALUE(current->size);
		if (adress >= start && adress < end)
			return (1);
		current = current->next;
	}
	return (0);
}