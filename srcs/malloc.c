/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 19:48:46 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/29 19:05:45 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

t_data g_data = {0};
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_pagesize = 0;

//void *ft_malloc(size_t size)
//{
//	if (g_pagesize == 0)
//	{
//		if (init_data() == -1)
//			return (NULL);
//	}
//	if (size == 0)
//		return (NULL);

//	if (size < 32 - sizeof(t_block))
//	{
//		t_block *ret_ptr;
//		ret_ptr = g_data.free_blocks.blocks[BLOCKS_32];
//		if (ret_ptr == NULL)
//			return NULL;
//		g_data.free_blocks.blocks[BLOCKS_32] = ret_ptr->next;
//		g_data.free_blocks.size_blocks[BLOCKS_32]--;
//		t_block *last_block;
//		last_block = g_data.allocated_blocks.blocks[BLOCKS_32];
//		while (last_block && last_block->next)
//			last_block = last_block->next;
//		if (!last_block)
//			g_data.allocated_blocks.blocks[BLOCKS_32] = ret_ptr;
//		else
//			last_block->next = ret_ptr;
//		ret_ptr->next = NULL;
//		g_data.allocated_blocks.size_blocks[BLOCKS_32]++;
//		return ((void *)ret_ptr + sizeof(t_block));
//	}
//	(void)size;
//	return (NULL);
//}

#include <stdio.h>

void* alloc_block(int index_size)
{
	t_block *ret_ptr;

	if (!g_data.free_blocks.size_blocks[index_size])
		return (pthread_mutex_unlock(&g_mutex), NULL);
	ret_ptr = g_data.free_blocks.blocks[index_size];
	g_data.free_blocks.blocks[index_size] = ret_ptr->next;
	ret_ptr->next = g_data.allocated_blocks.blocks[index_size];
	g_data.allocated_blocks.blocks[index_size] = ret_ptr;
	g_data.free_blocks.size_blocks[index_size]--;
	g_data.allocated_blocks.size_blocks[index_size]++;
	pthread_mutex_unlock(&g_mutex);
	return ((void *)ret_ptr + sizeof(t_block));
}

void* alloc_big_block(size_t size)
{
	(void)size;
	pthread_mutex_unlock(&g_mutex);
	return (NULL);
}

void *ft_malloc(size_t size)
{
	int index_size = 0;
	pthread_mutex_lock(&g_mutex);
	if (g_pagesize == 0)
	{
		if (init_data() == -1)
		{
			pthread_mutex_unlock(&g_mutex);
			return (NULL);
		}
	}
	if (size == 0)
	{
		pthread_mutex_unlock(&g_mutex);
		return (NULL);
	}

	index_size = size_to_size_index(size + sizeof(t_block));

	if (index_size == -1 && size > 1024)
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