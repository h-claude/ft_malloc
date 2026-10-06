/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 15:22:55 by hclaude           #+#    #+#             */
/*   Updated: 2026/10/06 17:52:43 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static void ft_putchar(char c)
{
	write(1, &c, 1);
}

static void ft_putstr(const char *s)
{
	while (*s)
		write(1, s++, 1);
}

static void ft_putnbr(size_t n)
{
	char c;

	if (n >= 10)
		ft_putnbr(n / 10);
	c = '0' + (n % 10);
	write(1, &c, 1);
}

static void ft_puthex(unsigned long n)
{
	char *hex = "0123456789ABCDEF";

	if (n >= 16)
		ft_puthex(n / 16);
	write(1, &hex[n % 16], 1);
}

static void print_addr(void *ptr)
{
	ft_putstr("0x");
	ft_puthex((unsigned long)ptr);
}


static int arena_has_blocks(t_arena *arena, int start, int end)
{
	void    *a_start;
	void    *a_end;
	t_block *b;

	a_start = (char *)arena + sizeof(t_arena);
	a_end   = (char *)arena + arena->size;
	for (int i = start; i <= end; i++)
	{
		b = g_data.allocated_blocks.blocks[i];
		while (b)
		{
			if ((void *)b >= a_start && (void *)b < a_end)
				return (1);
			b = b->next;
		}
	}
	return (0);
}

static t_arena *next_arena(t_arena *prev, int start, int end)
{
	t_arena *arena;
	t_arena *best;

	best = NULL;
	arena = g_data.arena;
	while (arena)
	{
		if ((!prev || arena > prev) && (!best || arena < best)
			&& arena_has_blocks(arena, start, end))
			best = arena;
		arena = arena->next;
	}
	return (best);
}

static t_block *next_block_in_arena(t_arena *arena, t_block *prev, int start, int end)
{
	void    *a_start;
	void    *a_end;
	t_block *b;
	t_block *best;

	a_start = (char *)arena + sizeof(t_arena);
	a_end   = (char *)arena + arena->size;
	best = NULL;
	for (int i = start; i <= end; i++)
	{
		b = g_data.allocated_blocks.blocks[i];
		while (b)
		{
			if ((void *)b >= a_start && (void *)b < a_end
				&& (!prev || b > prev) && (!best || b < best))
				best = b;
			b = b->next;
		}
	}
	return (best);
}

static t_block *next_big_block(t_block *prev)
{
	t_block *curr;
	t_block *best;

	best = NULL;
	curr = g_data.big_blocks.blocks;
	while (curr)
	{
		if ((!prev || curr > prev) && (!best || curr < best))
			best = curr;
		curr = curr->next;
	}
	return (best);
}

static size_t print_block(void *start_addr, size_t usable)
{
	print_addr(start_addr);
	ft_putstr(" - ");
	print_addr((char *)start_addr + usable);
	ft_putstr(" : ");
	ft_putnbr(usable);
	ft_putstr(" bytes\n");
	return (usable);
}

static size_t print_zone(const char *label, int start, int end)
{
	t_arena *arena;
	t_block *b;
	size_t   total;

	total = 0;
	arena = next_arena(NULL, start, end);
	while (arena)
	{
		ft_putstr(label);
		print_addr((void *)arena);
		ft_putchar('\n');
		b = next_block_in_arena(arena, NULL, start, end);
		while (b)
		{
			total += print_block((char *)b + sizeof(t_block),
					SIZE_VALUE(b->size) - sizeof(t_block));
			b = next_block_in_arena(arena, b, start, end);
		}
		arena = next_arena(arena, start, end);
	}
	return (total);
}

static size_t print_tiny(void)
{
	return (print_zone("TINY : ", BLOCKS_32, BLOCKS_128));
}

static size_t print_small(void)
{
	return (print_zone("SMALL : ", BLOCKS_256, BLOCKS_1040));
}

static size_t print_large(void)
{
	size_t total;
	t_block *curr;

	total = 0;
	curr = next_big_block(NULL);
	while (curr)
	{
		ft_putstr("LARGE : ");
		print_addr((void *)curr);
		ft_putchar('\n');
		total += print_block((char *)curr + sizeof(t_block), curr->size);
		curr = next_big_block(curr);
	}
	return (total);
}

void show_alloc_mem(void)
{
	size_t total;

	total = 0;
	pthread_mutex_lock(&g_mutex);
	total += print_tiny();
	total += print_small();
	total += print_large();
	ft_putstr("Total : ");
	ft_putnbr(total);
	ft_putstr(" bytes\n");
	pthread_mutex_unlock(&g_mutex);
}
