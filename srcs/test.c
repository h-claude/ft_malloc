#include <unistd.h>
#include <stdio.h>
#include "../includes/ft_malloc.h"

#define POOL_SIZE 64

int	main(void)
{
	void	*pool[POOL_SIZE];
	int		i;
	int		tick;

	printf("ft_malloc debug demo — Ctrl-C to stop\n\n");

	i = 0;
	while (i < POOL_SIZE)
		pool[i++] = NULL;

	tick = 0;
	while (1)
	{
		i = tick % POOL_SIZE;

		/* free the slot if already occupied */
		if (pool[i])
		{
			free(pool[i]);
			pool[i] = NULL;
		}

		/* allocate a mix of tiny, medium and large blocks */
		if (tick % 5 == 0)
			pool[i] = malloc(8000);        /* large */
		else if (tick % 3 == 0)
			pool[i] = malloc(300 + i * 3); /* medium */
		else
			pool[i] = malloc(10 + i);      /* tiny */

		tick++;
		usleep(80000); /* 80 ms per tick, counter refreshes every 100 ms */
	}
	return (0);
}
