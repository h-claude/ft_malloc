/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:48:46 by hclaude           #+#    #+#             */
/*   Updated: 2026/06/03 17:24:49 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_data g_data = {0};
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int init_zone(int index)
{
	size_t	block_size;
	size_t	pages;
	size_t	total;
	void	*ptr;
	t_arena	*arena;

	block_size = block_size_for_index(index);
	pages = (sizeof(t_arena) + 100 * block_size + g_data.pagesize - 1) / g_data.pagesize;
	total = pages * g_data.pagesize;
	ptr = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
		return (-1);
	arena = (t_arena *)ptr;
	arena->size = total;
	arena->next = g_data.arena;
	g_data.arena = arena;
	g_data.zone_bump[index] = (char *)ptr + sizeof(t_arena);
	g_data.zone_end[index] = (char *)ptr + total;
	return (0);
}

static t_block *carve_bump(int index_size)
{
	t_block *block;

	if (!g_data.zone_bump[index_size] ||
		(char *)g_data.zone_bump[index_size] + block_size_for_index(index_size) > (char *)g_data.zone_end[index_size])
		return (NULL);
	block = (t_block *)g_data.zone_bump[index_size];
	block->size = SET_ALLOC(block_size_for_index(index_size));
	block->next = g_data.allocated_blocks.blocks[index_size];
	g_data.allocated_blocks.blocks[index_size] = block;
	g_data.allocated_blocks.size_blocks[index_size]++;
	g_data.zone_bump[index_size] = (char *)g_data.zone_bump[index_size] + block_size_for_index(index_size);
	return (block);
}

static void get_more_blocks(int index_size)
{
	t_block *tmp_block;
	t_block *new_block;
	int     tmp_index;

	tmp_index = index_size + 1;
	while (!g_data.free_blocks.size_blocks[index_size] && tmp_index < 5)
	{
		if (!g_data.free_blocks.size_blocks[tmp_index])
			tmp_index++;
		else
		{
			tmp_block = g_data.free_blocks.blocks[tmp_index];
			if (!tmp_block)
				break;
			g_data.free_blocks.blocks[tmp_index] = tmp_block->next;
			size_t block_size = SIZE_VALUE(tmp_block->size);
			if (block_size != block_size_for_index(tmp_index) || block_size < 2 * sizeof(t_block))
			{
				g_data.free_blocks.blocks[tmp_index] = tmp_block;
				break;
			}
			g_data.free_blocks.size_blocks[tmp_index]--;
			new_block = (t_block *)((char *)tmp_block + block_size / 2);
			tmp_block->size = SET_FREE(block_size / 2);
			new_block->size = SET_FREE(block_size / 2);
			tmp_index = size_to_size_index(block_size / 2);
			tmp_block->next = new_block;
			new_block->next = g_data.free_blocks.blocks[tmp_index];
			g_data.free_blocks.blocks[tmp_index] = tmp_block;
			g_data.free_blocks.size_blocks[tmp_index] += 2;
		}
	}
	if (!g_data.free_blocks.size_blocks[index_size])
		init_zone(index_size);
}

static t_block *pop_free_to_allocated(int index_size)
{
	t_block *block;

	block = g_data.free_blocks.blocks[index_size];
	g_data.free_blocks.blocks[index_size] = block->next;
	block->size = SET_ALLOC(block->size);
	block->next = g_data.allocated_blocks.blocks[index_size];
	g_data.allocated_blocks.blocks[index_size] = block;
	g_data.free_blocks.size_blocks[index_size]--;
	g_data.allocated_blocks.size_blocks[index_size]++;
	return (block);
}

static void *alloc_block(int index_size)
{
	t_block *ret_ptr;

	if (g_data.free_blocks.size_blocks[index_size])
		ret_ptr = pop_free_to_allocated(index_size);
	else
	{
		ret_ptr = carve_bump(index_size);
		if (!ret_ptr)
		{
			get_more_blocks(index_size);
			if (g_data.free_blocks.size_blocks[index_size])
				ret_ptr = pop_free_to_allocated(index_size);
			else
				ret_ptr = carve_bump(index_size);
		}
	}
	if (!ret_ptr)
		return (NULL);
	return ((char *)ret_ptr + sizeof(t_block));
}

static void *alloc_big_block(size_t size)
{
	void *ptr;
	t_block *header;

	ptr = mmap(NULL, size + sizeof(t_block), PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
		return (NULL);
	header = (t_block *)ptr;
	header->size = SET_ALLOC(size);
	header->next = g_data.big_blocks.blocks;
	g_data.big_blocks.blocks = header;
	g_data.big_blocks.size_blocks++;
	return ((char *)header + sizeof(t_block));
}

void *malloc_unlocked(size_t size)
{
	int index_size = 0;
	if (g_data.pagesize == 0)
	{
		if (init_data() == -1)
		{
			return (NULL);
		}
	}
	if (size == 0 || size > SIZE_MAX - sizeof(t_block))
	{
		return (NULL);
	}

	index_size = size_to_size_index(size + sizeof(t_block));

	if (index_size == -1 && size + sizeof(t_block) > 1040)
		return (alloc_big_block(size));
	else if (index_size == -1)
		return (NULL);

	return (alloc_block(index_size));
}

void *malloc(size_t size)
{
	void *ptr;

	pthread_mutex_lock(&g_mutex);
	ptr = malloc_unlocked(size);
	pthread_mutex_unlock(&g_mutex);
	return (ptr);
}

// Comment faire pour que j'ai une bonne allocation memoire en fonction de la taille

/*
Fist bah ouais si ta structure existe pas bah ca va pas ca veut dire que t'as pas de memoire allouee DONC
tu checks si t'as de la memoire POUR CA soit on fait un check de la taille qu'on nous donne soit on check tout le temps l'ensemble de la memoire

Je pense qu'il faut check si la struct existe, ensuite par rapport a la taille demande check si on a ce qu'il faut.
Premier cas si on a pas la struct on init
Deuxieme cas si on a pas la memoire
Soit on commence par le plus petit pour chercher a fusionner soit on cherche les plus grands pour les fragmenter

Quel est le plus opti chercher celui qui a le plus grand nombre par exemple 1000 de 32 OU par exemple prioriser celui qui est le moins utiliser par exemple
1024 0 utilise

faire une fonction qui va fragmenter/defragmenter la memoire en fonction du besoin

Est il possible de d'avoir un fonctionnement qui permet de faciliter la refragmentation de la memoire


note :

Quand j'alloue je mets pas a la fin mais au debut
Pour la fusion je check dans la memoire + sa taille pour voir si le prochain est libre ou non et dans ce cas le fusionner

Quand je free je dois fusionner avec le suivant ou le precedent si possible
quand j'alloue soit je prends le premier bloc libre soit je split
un bloc plus gros au dessus.

A chaque mmap je dois maj ma structure et enregistrer la page allouee pour m'aider a fusionner

ajouter dans chaque block un flag pour savoir si il est libre ou non

surement devoir ajouter des variables pour alligner mon header
*/