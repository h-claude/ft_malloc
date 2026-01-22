#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include "../includes/ft_malloc.h"

int main()
{
	char *str;

	printf("--- ETAT INITIAL ---\n");
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
	void *test;
	while ((test = ft_malloc(6)))
	{
		visualize_memory(0);
		if (!test)
			break;
	}
	while(1)
	{
	};
	return 0;
}