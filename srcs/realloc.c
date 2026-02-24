/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 15:18:07 by hclaude           #+#    #+#             */
/*   Updated: 2026/02/24 18:13:32 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

void *ft_realloc(void *ptr, size_t size)
{
	if (size == 0 || size + sizeof(t_block) > SIZE_MAX)
	{
		ft_free(ptr);
		return (NULL);
	}
	if (!ptr)
		return (ft_malloc(size));
	t_block *header = (t_block *)((char *)ptr - sizeof(t_block));
	size_t real_size = SIZE_VALUE(header->size);
	if (real_size >= size)
		return (ptr);
	void *new_ptr = ft_malloc(size);
	if (!new_ptr)
		return (NULL);
	memcpy(new_ptr, ptr, real_size);
	ft_free(ptr);
	return (new_ptr);
}