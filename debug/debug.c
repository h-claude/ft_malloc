/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 00:00:00 by hclaude           #+#    #+#             */
/*   Updated: 2026/04/14 00:00:00 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include <crt_externs.h>

static int	check_malloc_debug(void)
{
	char	**env;
	int		i;
	char	*e;
	char	*t;

	env = *_NSGetEnviron();
	if (!env)
		return (0);
	i = 0;
	while (env[i])
	{
		e = env[i];
		t = "MALLOC_DEBUG=1";
		while (*e && *t && *e == *t)
		{
			e++;
			t++;
		}
		if (*t == '\0')
			return (1);
		i++;
	}
	return (0);
}

static void	dbg_char(char c)
{
	write(2, &c, 1);
}

static void	dbg_str(const char *s)
{
	while (*s)
		write(2, s++, 1);
}

static void	dbg_num(size_t n)
{
	char	buf[24];
	int		len;

	len = 0;
	if (n == 0)
	{
		dbg_char('0');
		return ;
	}
	while (n > 0)
	{
		buf[len++] = '0' + (n % 10);
		n /= 10;
	}
	while (--len >= 0)
		dbg_char(buf[len]);
}

static void	dbg_mem(size_t bytes)
{
	if (bytes >= 1024 * 1024)
	{
		dbg_num(bytes / (1024 * 1024));
		dbg_char('.');
		dbg_num((bytes % (1024 * 1024)) * 10 / (1024 * 1024));
		dbg_str("MB");
	}
	else if (bytes >= 1024)
	{
		dbg_num(bytes / 1024);
		dbg_char('.');
		dbg_num((bytes % 1024) * 10 / 1024);
		dbg_str("KB");
	}
	else
	{
		dbg_num(bytes);
		dbg_char('B');
	}
}

static void	*debug_loop(void *arg)
{
	static const size_t	bucket_usable[6] = {16, 48, 112, 240, 496, 1008};
	size_t				malloc_calls;
	size_t				free_calls;
	size_t				live_mem;
	int					live_blocks;
	t_block				*b;
	int					i;

	(void)arg;
	while (1)
	{
		usleep(100000);
		pthread_mutex_lock(&g_mutex);
		malloc_calls = g_data.dbg_malloc_calls;
		free_calls = g_data.dbg_free_calls;
		live_blocks = (int)g_data.big_blocks.size_blocks;
		live_mem = 0;
		i = 0;
		while (i < 6)
		{
			live_blocks += g_data.allocated_blocks.size_blocks[i];
			live_mem += (size_t)g_data.allocated_blocks.size_blocks[i]
				* bucket_usable[i];
			i++;
		}
		b = g_data.big_blocks.blocks;
		while (b)
		{
			live_mem += SIZE_VALUE(b->size);
			b = b->next;
		}
		pthread_mutex_unlock(&g_mutex);
		dbg_str("\033[2K\r");
		dbg_str("[malloc] alloc:");
		dbg_num(malloc_calls);
		dbg_str("  free:");
		dbg_num(free_calls);
		dbg_str("  live:");
		dbg_num((size_t)live_blocks);
		dbg_str("  mem:");
		dbg_mem(live_mem);
	}
	return (NULL);
}

void	start_debug_thread(void)
{
	pthread_t	thread;

	if (!check_malloc_debug())
		return ;
	pthread_create(&thread, NULL, debug_loop, NULL);
	pthread_detach(thread);
}

__attribute__((constructor))
static void	debug_auto_init(void)
{
	start_debug_thread();
}
