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

/* Write n into buf at position *pos, return updated pos */
static int	buf_num(char *buf, int pos, size_t n)
{
	char	tmp[24];
	int		len;

	if (n == 0)
	{
		buf[pos++] = '0';
		return (pos);
	}
	len = 0;
	while (n > 0)
	{
		tmp[len++] = '0' + (n % 10);
		n /= 10;
	}
	while (--len >= 0)
		buf[pos++] = tmp[len];
	return (pos);
}

static int	buf_str(char *buf, int pos, const char *s)
{
	while (*s)
		buf[pos++] = *s++;
	return (pos);
}

static int	buf_mem(char *buf, int pos, size_t bytes)
{
	if (bytes >= 1024 * 1024)
	{
		pos = buf_num(buf, pos, bytes / (1024 * 1024));
		buf[pos++] = '.';
		pos = buf_num(buf, pos, (bytes % (1024 * 1024)) * 10 / (1024 * 1024));
		pos = buf_str(buf, pos, "MB");
	}
	else if (bytes >= 1024)
	{
		pos = buf_num(buf, pos, bytes / 1024);
		buf[pos++] = '.';
		pos = buf_num(buf, pos, (bytes % 1024) * 10 / 1024);
		pos = buf_str(buf, pos, "KB");
	}
	else
	{
		pos = buf_num(buf, pos, bytes);
		buf[pos++] = 'B';
	}
	return (pos);
}

static void	*debug_loop(void *arg)
{
	size_t				malloc_calls;
	size_t				free_calls;
	size_t				live_mem;
	int					live_blocks;
	int					i;
	char				line[128];
	int					len;

	(void)arg;
	while (1)
	{
		usleep(33333);
		malloc_calls = atomic_load_explicit(&g_data.dbg_malloc_calls, memory_order_relaxed);
		free_calls   = atomic_load_explicit(&g_data.dbg_free_calls,   memory_order_relaxed);
		live_mem     = atomic_load_explicit(&g_data.dbg_live_mem,     memory_order_relaxed);
		pthread_mutex_lock(&g_mutex);
		live_blocks = (int)g_data.big_blocks.size_blocks;
		i = 0;
		while (i < 6)
		{
			live_blocks += g_data.allocated_blocks.size_blocks[i];
			i++;
		}
		pthread_mutex_unlock(&g_mutex);

		/* build the full line in one buffer, then emit with a single write() */
		len = 0;
		len = buf_str(line, len, "\033[2K\r[malloc] mem:");
		len = buf_mem(line, len, live_mem);
		len = buf_str(line, len, "  live:");
		len = buf_num(line, len, (size_t)live_blocks);
		len = buf_str(line, len, "  alloc:");
		len = buf_num(line, len, malloc_calls);
		len = buf_str(line, len, "  free:");
		len = buf_num(line, len, free_calls);
		write(2, line, len);
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
