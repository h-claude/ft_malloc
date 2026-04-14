/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:48:46 by hclaude           #+#    #+#             */
/*   Updated: 2026/04/14 15:42:43 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_data g_data = {0};
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void get_more_blocks(int index_size)
{
	t_block *tmp_block;
	t_block *new_block;
	int tmp_index = index_size;

	tmp_index++;
	while (!g_data.free_blocks.size_blocks[index_size] && tmp_index < 6 && tmp_index > 0 && tmp_index != index_size)
	{
		if (!g_data.free_blocks.size_blocks[tmp_index])
			tmp_index++;
		else
		{
			tmp_block = g_data.free_blocks.blocks[tmp_index];
			if (!tmp_block)
				break;
			g_data.free_blocks.blocks[tmp_index] = tmp_block->next;
			g_data.free_blocks.size_blocks[tmp_index]--;
			size_t block_size = SIZE_VALUE(tmp_block->size);
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
		init_data();
}

void *alloc_block(int index_size)
{
	t_block *ret_ptr;

	if (!g_data.free_blocks.size_blocks[index_size])
	{
		get_more_blocks(index_size);
		if (!g_data.free_blocks.size_blocks[index_size])
		{
			return (pthread_mutex_unlock(&g_mutex), NULL);
		}
	}
	ret_ptr = g_data.free_blocks.blocks[index_size];
	g_data.free_blocks.blocks[index_size] = ret_ptr->next;
	ret_ptr->size = SET_ALLOC(ret_ptr->size);
	ret_ptr->next = g_data.allocated_blocks.blocks[index_size];
	g_data.allocated_blocks.blocks[index_size] = ret_ptr;
	g_data.free_blocks.size_blocks[index_size]--;
	g_data.allocated_blocks.size_blocks[index_size]++;
	pthread_mutex_unlock(&g_mutex);
	return ((void *)ret_ptr + sizeof(t_block));
}

void *alloc_big_block(size_t size)
{
	void *ptr;
	t_block *header;

	ptr = mmap(NULL, size + sizeof(t_block), PROT_READ | PROT_WRITE,
			   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED)
	{
		pthread_mutex_unlock(&g_mutex);
		return (NULL);
	}
	header = (t_block *)ptr;
	header->size = SET_ALLOC(size);
	header->next = g_data.big_blocks.blocks;
	g_data.big_blocks.blocks = header;
	g_data.big_blocks.size_blocks++;
	pthread_mutex_unlock(&g_mutex);
	return ((void *)header + sizeof(t_block));
}

void *malloc(size_t size)
{
	int index_size = 0;
	pthread_mutex_lock(&g_mutex);
	if (g_data.pagesize == 0)
	{
		if (init_data() == -1)
		{
			pthread_mutex_unlock(&g_mutex);
			return (NULL);
		}
	}
	if (size == 0 || size > SIZE_MAX - sizeof(t_block))
	{
		pthread_mutex_unlock(&g_mutex);
		return (NULL);
	}

	index_size = size_to_size_index(size + sizeof(t_block));

	if (index_size == -1 && size + sizeof(t_block) > 1024)
		return (alloc_big_block(size));
	else if (index_size == -1)
		return pthread_mutex_unlock(&g_mutex), NULL;

	return (alloc_block(index_size));
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