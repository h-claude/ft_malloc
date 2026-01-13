/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hclaude <hclaude@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 17:57:26 by hclaude           #+#    #+#             */
/*   Updated: 2026/01/13 20:43:38 by hclaude          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
# define FT_MALLOC_H

#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <pthread.h>

#define BLOCKS_16 0
#define BLOCKS_32 1
#define BLOCKS_64 2
#define BLOCKS_128 3
#define BLOCKS_256 4
#define BLOCKS_512 5
#define BLOCKS_1024 6

typedef struct s_block
{
	struct s_block		*next;
	size_t				size;
} t_block;

typedef struct s_free_blocks
{
	t_block*	blocks[7];
	int			size_blocks[7];
} t_free_blocks;

typedef struct s_allocated_blocks
{
	struct s_block			*blocks;
	int						size_blocks;
} t_allocated_blocks;

typedef struct s_data
{
	t_free_blocks			free_blocks;
	t_allocated_blocks		allocated_blocks;
} t_data;

static t_data			g_data = {0};
static pthread_mutex_t	g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_pagesize = 0;

void	*ft_malloc(size_t size);
void	ft_free(void *ptr);
void	*ft_realloc(void *ptr, size_t size);
void	*show_alloc_mem(void);

#endif