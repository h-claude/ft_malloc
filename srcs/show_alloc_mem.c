/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 15:22:55 by hclaude           #+#    #+#             */
/*   Updated: 2026/05/29 17:16:40 by hclaude          ###   ########.fr       */
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

static size_t print_zone(const char *label, int start, int end)
{
	t_arena *arena;
	size_t   total;

	total = 0;
	arena = g_data.arena;
	while (arena)
	{
		if (arena_has_blocks(arena, start, end))
		{
			ft_putstr(label);
			print_addr((void *)arena);
			ft_putchar('\n');
			for (int i = start; i <= end; i++)
			{
				t_block *b = g_data.allocated_blocks.blocks[i];
				while (b)
				{
					void   *a_start = (char *)arena + sizeof(t_arena);
					void   *a_end   = (char *)arena + arena->size;
					if ((void *)b >= a_start && (void *)b < a_end)
					{
						size_t usable = SIZE_VALUE(b->size) - sizeof(t_block);
						void  *start_addr = (char *)b + sizeof(t_block);
						void  *end_addr   = (char *)b + SIZE_VALUE(b->size);
						print_addr(start_addr);
						ft_putstr(" - ");
						print_addr(end_addr);
						ft_putstr(" : ");
						ft_putnbr(usable);
						ft_putstr(" bytes\n");
						total += usable;
					}
					b = b->next;
				}
			}
		}
		arena = arena->next;
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
	curr = g_data.big_blocks.blocks;
	if (!curr)
		return (0);
	while (curr)
	{
		size_t payload = SIZE_VALUE(curr->size);
		void *start = (void *)((char *)curr + sizeof(t_block));
		void *end = (void *)((char *)start + payload);
		ft_putstr("LARGE : ");
		print_addr((void *)curr);
		ft_putchar('\n');
		print_addr(start);
		ft_putstr(" - ");
		print_addr(end);
		ft_putstr(" : ");
		ft_putnbr(payload);
		ft_putstr(" bytes\n");
		total += payload;
		curr = curr->next;
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
