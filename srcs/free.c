/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 18:42:03 by hclaude           #+#    #+#             */
/*   Updated: 2026/02/04 19:10:17 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

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
	if (header->size > 1024)
	{
		t_block *big_curr;
		t_block *big_prev;
		size_t total_size;

		total_size = header->size + sizeof(t_block);
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
	size_t real_size = SIZE_VALUE(header->size);
	size_index = size_to_size_index(real_size);

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
	header->next = g_data.free_blocks.blocks[size_index];
	g_data.free_blocks.blocks[size_index] = header;
	g_data.free_blocks.size_blocks[size_index]++;
	pthread_mutex_unlock(&g_mutex);
}