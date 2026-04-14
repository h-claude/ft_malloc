/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 15:18:07 by hclaude           #+#    #+#             */
/*   Updated: 2026/04/14 16:20:31 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

void *realloc(void *ptr, size_t size)
{
	if (size == 0 || size + sizeof(t_block) > SIZE_MAX)
	{
		free(ptr);
		return (NULL);
	}
	if (!ptr)
		return (malloc(size));
	pthread_mutex_lock(&g_mutex);
	t_block *header = (t_block *)((char *)ptr - sizeof(t_block));
	size_t real_size = SIZE_VALUE(header->size);
	int is_big = Is_In_BigBlocks(ptr);
	pthread_mutex_unlock(&g_mutex);
	size_t usable = is_big ? real_size : real_size - sizeof(t_block);
	if (usable >= size)
		return (ptr);
	void *new_ptr = malloc(size);
	if (!new_ptr)
		return (NULL);
	memcpy(new_ptr, ptr, usable);
	free(ptr);
	return (new_ptr);
}