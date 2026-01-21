#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include "../includes/ft_malloc.h"

int main()
{
	void *test = mmap(NULL, 6 * sizeof(char), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (test == MAP_FAILED)
	{
		perror("mmap failed");
		return 1;
	}
	char *str = (char *)test;
	str[0] = 'H';
	str[1] = 'e';
	str[2] = 'l';
	str[3] = 'l';
	str[4] = 'o';
	str[5] = '\0';
	write(1, str, 6);
	munmap(test, 6 * sizeof(char));
	printf("\nsize of t_data: %zu\n", sizeof(t_data));
	printf("size of t_block: %zu\n", sizeof(t_block));
	printf("test : %d\n", g_data.free_blocks.size_blocks[BLOCKS_32]);
	test = mmap(NULL, 25 * sizeof(char), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (test == MAP_FAILED)
	{
		perror("mmap failed");
		return 1;
	}
	// g_data.free_blocks.blocks[BLOCKS_16] = (t_block*)test;
	char *str1 = (char *)test;
	char *str2 = (char *)test + sizeof(char) * 6;
	str1[0] = 'W';
	str1[1] = 'o';
	str1[2] = 'r';
	str1[3] = 'l';
	str1[4] = 'd';
	str1[5] = '\0';
	write(1, str1, 6);
	str2[0] = '!';
	str2[1] = '\n';
	str2[2] = '\0';
	write(1, str2, 2);
	munmap(test, 6 * sizeof(char));

	// test allocation strucutre
	g_pagesize = getpagesize();
	test = mmap(NULL, g_pagesize * 16, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (test == MAP_FAILED)
	{
		perror("mmap failed");
		return 1;
	}

	// Configuration de la repartition des 16 pages
	// Index: 0=32, 1=64, 2=128, 3=256, 4=512, 5=1024
	int pages_per_type[6] = {3, 3, 3, 3, 2, 2};
	int block_sizes[6] = {32, 64, 128, 256, 512, 1024};

	char *current_ptr = (char *)test;
	int type = 0;

	// Boucle sur les 6 types de blocs
	while (type < 6)
	{
		size_t size_for_this_type = pages_per_type[type] * g_pagesize;
		size_t bytes_processed = 0;

		// Initialisation du pointeur de tete pour ce type
		g_data.free_blocks.blocks[type] = (t_block *)current_ptr;
		g_data.free_blocks.size_blocks[type] = 0;

		// Decoupage de la zone dediee a ce type
		while (bytes_processed + block_sizes[type] <= size_for_this_type)
		{
			t_block *block = (t_block *)current_ptr;
			block->size = block_sizes[type];
			// Le prochain bloc est situe 'block_sizes[type]' plus loin
			block->next = (t_block *)(current_ptr + block_sizes[type]);

			current_ptr += block_sizes[type];
			bytes_processed += block_sizes[type];
			g_data.free_blocks.size_blocks[type]++;
		}

		// Le dernier bloc de la liste doit pointer vers NULL (et non vers le debut de la categorie suivante)
		// On recule d'un cran pour recuperer le dernier bloc traite
		t_block *last_block = (t_block *)(current_ptr - block_sizes[type]);
		last_block->next = NULL;

		// Si il reste un petit bout de memoire inutilisable a la fin de la page (padding), on le saute
		// (current_ptr est deja au bon endroit pour la prochaine categorie si tout est aligne,
		// mais avec des tailles bizarres, il faudrait aligner sur la page suivante)
		// Ici, on force l'alignement sur le debut de la prochaine section de pages
		// Note: Dans cet exemple simplifie, on suppose que current_ptr est arrive pile a la fin ou presque.
		// Pour etre rigoureux, on devrait recalculer current_ptr basé sur le nombre de pages consommées.

		type++;
	}

	printf("Number of free blocks of size 32: %d\n", g_data.free_blocks.size_blocks[BLOCKS_32]);
	printf("Number of free blocks of size 1024: %d\n", g_data.free_blocks.size_blocks[BLOCKS_1024]);

	munmap(test, g_pagesize * 16);

	return 0;
}