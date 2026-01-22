/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:48:46 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/22 19:20:54 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_data g_data = {0};
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_pagesize = 0;

void *ft_malloc(size_t size)
{
	if (g_pagesize == 0)
	{
		if (init_data() == -1)
			return (NULL);
	}
	if (size == 0)
		return (NULL);

	if (size < 32 - sizeof(t_block))
	{
		t_block *ret_ptr;
		ret_ptr = g_data.free_blocks.blocks[BLOCKS_32];
		if (ret_ptr == NULL)
			return NULL;
		g_data.free_blocks.blocks[BLOCKS_32] = ret_ptr->next;
		g_data.free_blocks.size_blocks[BLOCKS_32]--;
		t_block *last_block;
		last_block = g_data.allocated_blocks.blocks[BLOCKS_32];
		while (last_block && last_block->next)
			last_block = last_block->next;
		if (!last_block)
			g_data.allocated_blocks.blocks[BLOCKS_32] = ret_ptr;
		else
			last_block->next = ret_ptr;
		ret_ptr->next = NULL;
		g_data.allocated_blocks.size_blocks[BLOCKS_32]++;
		return ((void *)ret_ptr + sizeof(t_block));
	}
	(void)size;
	return (NULL);
}