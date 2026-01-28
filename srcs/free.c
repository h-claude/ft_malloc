/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 18:42:03 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/28 18:19:14 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

void ft_free(void *ptr)
{
	t_block *header;
	int size_index;
	t_block *last_block;
	t_block *curr;
	t_block *prev;

	if (!ptr)
		return;

	header = (t_block *)((char *)ptr - sizeof(t_block));
	size_index = size_to_size_index(header->size);

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

	last_block = get_last_block(size_index, 0);
	if (last_block == NULL)
		g_data.free_blocks.blocks[size_index] = header;
	else
		last_block->next = header;

	header->next = NULL;
	g_data.free_blocks.size_blocks[size_index]++;
	pthread_mutex_unlock(&g_mutex);
}