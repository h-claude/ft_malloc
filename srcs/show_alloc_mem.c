/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/19 15:22:55 by hclaude           #+#    #+#             */
/*   Updated: 2026/02/19 16:03:38 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

static void ft_putchar(char c)
{
	write(1, &c, 1);
}

static void ft_putstr(char *s)
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

static size_t print_block_list(t_block *head)
{
	size_t total;

	total = 0;
	while (head)
	{
		size_t block_size = SIZE_VALUE(head->size);
		void *start = (void *)((char *)head + sizeof(t_block));
		void *end = (void *)((char *)head + block_size);
		print_addr(start);
		ft_putstr(" - ");
		print_addr(end);
		ft_putstr(" : ");
		ft_putnbr(block_size);
		ft_putstr(" bytes\n");
		total += block_size;
		head = head->next;
	}
	return (total);
}

static int has_alloc(int start, int end)
{
	for (int i = start; i <= end; i++)
		if (g_data.allocated_blocks.size_blocks[i] > 0)
			return (1);
	return (0);
}

static size_t print_tiny(void)
{
	size_t total;
	t_arena *arena;

	total = 0;
	if (!has_alloc(BLOCKS_32, BLOCKS_128))
		return (0);
	arena = g_data.arena;
	while (arena)
	{
		ft_putstr("TINY : ");
		print_addr((void *)arena);
		ft_putchar('\n');
		arena = arena->next;
	}
	for (int i = BLOCKS_32; i <= BLOCKS_128; i++)
		total += print_block_list(g_data.allocated_blocks.blocks[i]);
	return (total);
}

static size_t print_small(void)
{
	size_t total;
	t_arena *arena;

	total = 0;
	if (!has_alloc(BLOCKS_256, BLOCKS_1024))
		return (0);
	arena = g_data.arena;
	while (arena)
	{
		ft_putstr("SMALL : ");
		print_addr((void *)arena);
		ft_putchar('\n');
		arena = arena->next;
	}
	for (int i = BLOCKS_256; i <= BLOCKS_1024; i++)
		total += print_block_list(g_data.allocated_blocks.blocks[i]);
	return (total);
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
		ft_putstr(" - ");
		print_addr(end);
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

void *show_alloc_mem(void)
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
	return (NULL);
}