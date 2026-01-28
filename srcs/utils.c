/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:49:24 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/28 18:25:37 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

int	size_to_size_index(size_t size)
{
	if (size <= 0 || size > 1024)
		return (-1);

	if (size <= 32)
		return (BLOCKS_32);

	size_t rounded = 32;
	while (rounded < size)
		rounded <<= 1;

	return (63 - __builtin_clzl(rounded) - 5);
}

t_block* get_last_block(int size_index, int is_allocated)
{
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

void page_number_distributor(void* current_ptr)
{
	int i = 0;
	int block_sizes[] = {32, 64, 128, 256, 512, 1024};
	int base_pages = DEFAULT_PAGE_COUNT / 6;
	int remainder = DEFAULT_PAGE_COUNT % 6;

	while (i < 6)
	{
		int pages = base_pages + (i < remainder);

		if (pages > 0)
		{
			size_t zone_size = pages * g_pagesize;
			size_t processed = 0;

			g_data.free_blocks.blocks[i] = (t_block *)current_ptr;
			while (processed + block_sizes[i] <= zone_size) // faut que tu m'expliques
			{
				t_block *block = (t_block *)current_ptr;
				block->size = block_sizes[i];
				block->next = (t_block *)(current_ptr + block_sizes[i]);
				current_ptr += block_sizes[i];
				processed += block_sizes[i];
				g_data.free_blocks.size_blocks[i]++;
			}
			((t_block *)(current_ptr - block_sizes[i]))->next = NULL;
		}
		i++;
	}
}

int init_data()
{
	void *ptr;

	if (g_pagesize == 0)
	{
		g_pagesize = getpagesize();

	}
	if (g_data.free_blocks.size_blocks[BLOCKS_32] +
			g_data.free_blocks.size_blocks[BLOCKS_64] +
			g_data.free_blocks.size_blocks[BLOCKS_128] +
			g_data.free_blocks.size_blocks[BLOCKS_256] +
			g_data.free_blocks.size_blocks[BLOCKS_512] +
			g_data.free_blocks.size_blocks[BLOCKS_1024] ==
		0)
	{
		ptr = mmap(NULL, g_pagesize * DEFAULT_PAGE_COUNT, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (ptr == MAP_FAILED)
			return (-1);
		page_number_distributor(ptr);
	}
	return (0);
}


