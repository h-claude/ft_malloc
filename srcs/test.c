#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include "../includes/ft_malloc.h"

int main()
{
	void* test = mmap(NULL, 6 * sizeof(char), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (test == MAP_FAILED)
	{
		perror("mmap failed");
		return 1;
	}
	char* str = (char*)test;
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
	printf("test : %d\n", g_data.free_blocks.size_blocks[BLOCKS_16]);
	return 0;
}