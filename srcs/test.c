#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include "../includes/ft_malloc.h"
#include <stdalign.h>

int main()
{
	char *str;

	printf("--- ETAT INITIAL ---\n");
	printf("taille de t_block: %zu\n", sizeof(t_block));
	printf("taille de t_page: %zu\n", sizeof(t_arena));
	// On appelle malloc(0) juste pour forcer le init_data() si besoin,
	// ou on peut appeler init_data() directement si on le rendait public.
	// Ici, le premier malloc fera l'init.

	str = ft_malloc(sizeof(char) * 6);

	str[0] = 'H';
	str[1] = 'e';
	str[2] = 'l';
	str[3] = 'l';
	str[4] = 'o';
	str[5] = 0;
	printf("String allocated: %s\n", str);
	visualize_memory(0);
	ft_free(str);
	visualize_memory(0);
	char *str2 = ft_malloc(sizeof(char) * 6);
	ft_free(str);
	visualize_memory(0);
	ft_free(str2);
	visualize_memory(0);

	//// ===== TEST FRAGMENTATION + DEFRAGMENTATION =====
	//printf("\n===== TEST FRAG/DEFRAG =====\n");
	//void *ptrs[2000];
	//int i;

	//// 1) 2000 mallocs de 20 bytes -> force fragmentation + init_data si besoin
	//printf("\n[1] 2000 malloc(20)...\n");
	//i = 0;
	//while (i < 2000)
	//{
	//	ptrs[i] = ft_malloc(20);
	//	i++;
	//}
	//visualize_memory(0);

	//// 2) Free 1 bloc sur 2 -> crée des trous partout
	//printf("\n[2] Free 1 block out of 2 (1000 frees)...\n");
	//i = 0;
	//while (i < 2000)
	//{
	//	ft_free(ptrs[i]);
	//	ptrs[i] = NULL;
	//	i += 2;
	//}
	//visualize_memory(0);

	//// 3) Free le reste -> tout doit revenir, defrag doit merger
	//printf("\n[3] Free all remaining blocks (1000 frees)...\n");
	//i = 1;
	//while (i < 2000)
	//{
	//	ft_free(ptrs[i]);
	//	ptrs[i] = NULL;
	//	i += 2;
	//}
	//visualize_memory(0);

	//// 4) Re-alloue 2000 pour voir si les blocs mergés sont réutilisés
	//printf("\n[4] 2000 malloc(20) again after defrag...\n");
	//i = 0;
	//while (i < 2000)
	//{
	//	ptrs[i] = ft_malloc(20);
	//	i++;
	//}
	//visualize_memory(0);

	//// 5) Free tout d'un coup
	//printf("\n[5] Free all 2000 blocks...\n");
	//i = 0;
	//while (i < 2000)
	//{
	//	ft_free(ptrs[i]);
	//	i++;
	//}
	//visualize_memory(0);

	ft_malloc(6);
	ft_malloc(6);
	ft_malloc(6);
	ft_malloc(6);
	ft_malloc(600);
	ft_malloc(400);
	ft_malloc(6000);
	ft_malloc(6000);
	int counter = 0;
	while (counter < 100)
	{
		ft_malloc(600);
		counter++;
	}
	show_alloc_mem();

	return 0;
}