#include "ft_malloc.h"
#include <stdio.h>

// Couleurs pour le terminal
#define RESET "\033[0m"
#define RED "\033[31m"	  // Pour les blocs alloués
#define GREEN "\033[32m"  // Pour les blocs libres
#define YELLOW "\033[33m" // Pour les titres
#define BLUE "\033[34m"	  // Pour les adresses
#define CYAN "\033[36m"	  // Pour les infos

static void print_block_chain(t_block *head, char *color)
{
	t_block *current = head;
	int count = 0;

	if (!current)
	{
		printf("%s(NULL)%s\n", color, RESET);
		return;
	}

	while (current)
	{
		// Affiche : [ Adr | Size ] ->
		printf("%s[%p | %zu]%s", color, (void *)current, SIZE_VALUE(current->size), RESET);

		current = current->next;
		if (current)
			printf(" -> ");

		// Retour a la ligne tous les 5 blocs pour ne pas casser l'affichage
		count++;
		if (count % 5 == 0 && current)
			printf("\n\t\t      ");
	}
	printf(" -> %sNULL%s\n", color, RESET);
}

void visualize_memory(int detailed)
{
	int sizes[] = {32, 64, 128, 256, 512, 1024};
	int i = 0;

	printf("\n%s===============================================================%s\n", CYAN, RESET);
	printf("%s                   MEMORY VISUALIZER                           %s\n", CYAN, RESET);
	printf("%s===============================================================%s\n", CYAN, RESET);

	if (g_pagesize == 0)
	{
		printf("%s[!] Data not initialized yet (g_pagesize is 0)%s\n", RED, RESET);
		return;
	}

	if (!detailed)
	{
		printf("%s%-10s | %-15s | %-15s%s\n", YELLOW, "Zone Size", "Free Blocks", "Alloc Blocks", RESET);
		printf("---------------------------------------------------\n");
	}

	while (i < 6)
	{
		if (detailed)
		{
			printf("\n%s>>> ZONE: %d bytes blocks %s\n", YELLOW, sizes[i], RESET);
			// 1. Affichage des blocs LIBRES
			printf("  %s[FREE LIST]%s (%d blocks)\n", GREEN, RESET, g_data.free_blocks.size_blocks[i]);
			printf("  └── ");
			print_block_chain(g_data.free_blocks.blocks[i], GREEN);
			// 2. Affichage des blocs ALLOUÉS
			printf("  %s[ALLOC LIST]%s (%d blocks)\n", RED, RESET, g_data.allocated_blocks.size_blocks[i]);
			printf("  └── ");
			print_block_chain(g_data.allocated_blocks.blocks[i], RED);
		}
		else
		{
			printf("%-10d | %s%-15d%s | %s%-15d%s\n", sizes[i],
				   GREEN, g_data.free_blocks.size_blocks[i], RESET,
				   RED, g_data.allocated_blocks.size_blocks[i], RESET);
		}
		i++;
	}
	if (detailed)
	{
		printf("\n%s>>> ZONE: > 1024 bytes blocks %s\n", YELLOW, RESET);
		printf("  %s[ALLOC LIST]%s (%d blocks)\n", RED, RESET, g_data.big_blocks.size_blocks);
		printf("  └── ");
		print_block_chain(g_data.big_blocks.blocks, RED);
	}
	else
	{
		printf("%-10s | %-15s | %s%-15d%s\n", ">1024",
			   "-",
			   RED, g_data.big_blocks.size_blocks, RESET);
	}
	printf("\n%s===============================================================%s\n\n", CYAN, RESET);
}