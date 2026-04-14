ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME		= libft_malloc_$(HOSTTYPE).so
LINK_NAME	= libft_malloc.so

SRCS		= srcs/malloc.c \
			  srcs/free.c \
			  srcs/realloc.c \
			  srcs/utils.c \
			  srcs/show_alloc_mem.c

CC			= gcc
CFLAGS		= -Wall -Wextra -Werror -fPIC -Iincludes -g3 -fvisibility=hidden

ifdef DEBUG
SRCS		+= debug/debug.c
CFLAGS		+= -DMALLOC_DEBUG_BUILD
endif

OBJS_DIR	= objs
OBJS		= $(patsubst %.c,$(OBJS_DIR)/%.o,$(notdir $(SRCS)))
LDFLAGS		= -shared

all:		$(OBJS_DIR) $(NAME) $(LINK_NAME)

$(NAME):	$(OBJS)
			$(CC) $(LDFLAGS) -o $(NAME) $(OBJS)

$(LINK_NAME):	$(NAME)
			ln -sf $(NAME) $(LINK_NAME)

$(OBJS_DIR):
			mkdir -p $(OBJS_DIR)

$(OBJS_DIR)/%.o:	srcs/%.c
			$(CC) $(CFLAGS) -c $< -o $@

$(OBJS_DIR)/%.o:	debug/%.c
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			rm -f $(OBJS)

fclean:		clean
			rm -rf $(OBJS_DIR)
			rm -f $(NAME)
			rm -f $(LINK_NAME)

test:		all
			$(CC) -Wall -Wextra -Iincludes -o run_tests test_malloc.c -L. -lft_malloc -lpthread
			DYLD_LIBRARY_PATH=. LD_LIBRARY_PATH=. ./run_tests

re:			fclean all

.PHONY:		all clean fclean re test