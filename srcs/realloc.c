/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 15:18:07 by hclaude           #+#    #+#             */
/*   Updated: 2026/10/06 17:50:56 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static void	ft_memcpy(void *dst, const void *src, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		((char *)dst)[i] = ((const char *)src)[i];
		i++;
	}
}

static int	is_valid_block(t_block *header, int is_big)
{
	t_block	*curr;
	int		size_index;

	if (is_big)
		curr = g_data.big_blocks.blocks;
	else
	{
		if (!Is_In_ArenA(header))
			return (0);
		size_index = size_to_size_index(SIZE_VALUE(header->size));
		if (size_index == -1)
			return (0);
		curr = g_data.allocated_blocks.blocks[size_index];
	}
	while (curr && curr != header)
		curr = curr->next;
	return (curr != NULL);
}

void *realloc(void *ptr, size_t size)
{
	void *new_ptr;
	t_block *header;
	size_t usable;
	int is_big;

	if (size == 0 || size + sizeof(t_block) > SIZE_MAX)
	{
		free(ptr);
		return (NULL);
	}
	if (!ptr)
		return (malloc(size));
	pthread_mutex_lock(&g_mutex);
	header = (t_block *)((char *)ptr - sizeof(t_block));
	is_big = Is_In_BigBlocks(ptr);
	if (!is_valid_block(header, is_big))
	{
		pthread_mutex_unlock(&g_mutex);
		return (NULL);
	}
	if (is_big)
		usable = header->size;
	else
		usable = SIZE_VALUE(header->size) - sizeof(t_block);
	if (usable >= size)
	{
		pthread_mutex_unlock(&g_mutex);
		return (ptr);
	}
	new_ptr = malloc_unlocked(size);
	if (!new_ptr)
	{
		pthread_mutex_unlock(&g_mutex);
		return (NULL);
	}
	ft_memcpy(new_ptr, ptr, usable);
	free_unlocked(ptr);
	pthread_mutex_unlock(&g_mutex);
	return (new_ptr);
}