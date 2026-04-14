/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:49:24 by hclaude           #+#    #+#             */
/*   Updated: 2026/04/14 15:42:59 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

int size_to_size_index(size_t size)
{
	if (size == 0 || size > 1024)
		return (-1);

	if (size <= 32)
		return (BLOCKS_32);

	size_t rounded = 32;
	while (rounded < size)
		rounded <<= 1;

	return (63 - __builtin_clzl(rounded) - 5);
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
	int size_index;

	if (!block)
		return (NULL);

	size_index = size_to_size_index(SIZE_VALUE(block->size));
	prev = get_previous_block(block, size_index, is_allocated);

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

void page_number_distributor(void *current_ptr)
{
	int i = 0;
	short block_sizes[] = {32, 64, 128, 256, 512, 1024};
	int base_pages = DEFAULT_PAGE_COUNT / 6;
	int remainder = DEFAULT_PAGE_COUNT % 6;

	if (!current_ptr || g_data.pagesize <= 0)
		return;

	while (i < 6)
	{
		int pages = base_pages + (i < remainder);

		if (pages > 0)
		{
			t_block *old_head = g_data.free_blocks.blocks[i];
			t_block *new_head = (t_block *)current_ptr;
			size_t zone_size = pages * g_data.pagesize;
			size_t processed = 0;
			int added_blocks = 0;

			while (processed + block_sizes[i] <= zone_size) // faut que tu m'expliques
			{
				t_block *block = (t_block *)current_ptr;
				block->size = SET_FREE(block_sizes[i]);
				block->next = (t_block *)(current_ptr + block_sizes[i]);
				current_ptr += block_sizes[i];
				processed += block_sizes[i];
				added_blocks++;
			}
			((t_block *)(current_ptr - block_sizes[i]))->next = NULL;
			if (added_blocks > 0)
			{
				t_block *new_tail = (t_block *)(current_ptr - block_sizes[i]);
				new_tail->next = old_head;
				g_data.free_blocks.blocks[i] = new_head;
			}
			g_data.free_blocks.size_blocks[i] += added_blocks;
		}
		i++;
	}
}

int init_data()
{
	void *ptr;
	t_arena *last_arena;

	if (g_data.pagesize == 0)
	{
		g_data.pagesize = getpagesize();
		g_data.arena = NULL;
		g_data.big_blocks.blocks = NULL;
		g_data.big_blocks.size_blocks = 0;
	}
	ptr = mmap(NULL, g_data.pagesize * DEFAULT_PAGE_COUNT + sizeof(t_arena), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
		return (-1);
	last_arena = g_data.arena;
	g_data.arena = (t_arena *)ptr;
	g_data.arena->next = last_arena;
	ptr += sizeof(t_arena);
	page_number_distributor(ptr);
	//}
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
		void *start = (void *)arena_adress + sizeof(t_arena);
		void *end = (void *)arena_adress + ARENA_SIZE;
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
		void *start = (void *)current + sizeof(t_block);
		void *end = (void *)current + sizeof(t_block) + SIZE_VALUE(current->size);
		if (adress >= start && adress < end)
			return (1);
		current = current->next;
	}
	return (0);
}