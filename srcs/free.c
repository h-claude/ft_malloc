/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 18:42:03 by hclaude           #+#    #+#             */
/*   Updated: 2026/02/24 18:35:52 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

void free_big_block(t_block *header)
{
	t_block *big_curr;
	t_block *big_prev;
	size_t total_size;

	total_size = SIZE_VALUE(header->size) + sizeof(t_block);
	pthread_mutex_lock(&g_mutex);
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
	pthread_mutex_unlock(&g_mutex);
	munmap(header, total_size);
	return;
}

void ft_free(void *ptr)
{
	t_block *header;
	int size_index;
	t_block *curr;
	t_block *prev;

	if (!ptr)
		return;

	header = (t_block *)((char *)ptr - sizeof(t_block));
	if (!header)
		return;

	if (!Is_In_ArenA(header))
		return;

	if (!Is_In_BigBlocks(header))
		return;

	if (header->size == 0)
		return;
	if (header->size > 1024)
	{
		free_big_block(header);
		return;
	}

	if (IS_FREE(header->size))
		return;

	size_t real_size = SIZE_VALUE(header->size);
	size_index = size_to_size_index(real_size);

	//if (SIZE_VALUE(SIZE_VALUE(header->size)) > 1024)
	//{
	//	free_big_block(header);
	//	return;
	//}


	if (size_index == -1)
		return;

	pthread_mutex_lock(&g_mutex);
	curr = g_data.allocated_blocks.blocks[size_index];
	prev = NULL;

	while (curr && curr != header)
	{
		prev = curr;
		curr = curr->next;
	}

	if (curr == NULL)
		return (void)pthread_mutex_unlock(&g_mutex);

	if (prev)
		prev->next = curr->next;
	else
		g_data.allocated_blocks.blocks[size_index] = curr->next;

	g_data.allocated_blocks.size_blocks[size_index]--;

	header->size = SET_FREE(header->size);
	t_block *next_header = (t_block *)((char *)header + SIZE_VALUE(header->size));
	int free_count = g_data.free_blocks.size_blocks[size_index];

	if (free_count > MIN_BLOCKS_TO_DEFRAG && Is_In_ArenA(next_header))
	{
		if (SIZE_VALUE(header->size) == SIZE_VALUE(next_header->size) && IS_FREE(next_header->size) && SIZE_VALUE(header->size) <= 512)
		{
			remove_block(next_header, 0);
			header->size = SIZE_VALUE(header->size) * 2;
			SET_FREE(header->size);
			size_index = size_to_size_index(SIZE_VALUE(header->size));
		}
	}

	header->next = g_data.free_blocks.blocks[size_index];
	g_data.free_blocks.blocks[size_index] = header;
	g_data.free_blocks.size_blocks[size_index]++;
	pthread_mutex_unlock(&g_mutex);
}