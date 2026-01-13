/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:49:24 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/13 21:03:41 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

int init_data()
{
	void *ptr;

	if (g_pagesize == 0)
		g_pagesize = getpagesize();
	if (pthread_mutex_lock(&g_mutex) != 0)
		return (-1);
	if (g_data.free_blocks.size_blocks[BLOCKS_16] +
		g_data.free_blocks.size_blocks[BLOCKS_32] +
		g_data.free_blocks.size_blocks[BLOCKS_64] +
		g_data.free_blocks.size_blocks[BLOCKS_128] +
		g_data.free_blocks.size_blocks[BLOCKS_256] +
		g_data.free_blocks.size_blocks[BLOCKS_512] +
		g_data.free_blocks.size_blocks[BLOCKS_1024] == 0)
	{
		ptr = mmap(NULL, g_pagesize * 16, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	}
	return (0);
}