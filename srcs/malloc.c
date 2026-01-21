/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:48:46 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/20 16:15:17 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_data g_data = {0};
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_pagesize = 0;

void *ft_malloc(size_t size)
{
	(void)size;
	return (NULL);
}