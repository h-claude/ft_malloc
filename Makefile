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

OBJS_DIR	= objs
OBJS		= $(SRCS:srcs/%.c=$(OBJS_DIR)/%.o)

CC			= gcc
CFLAGS		= -Wall -Wextra -Werror -fPIC -Iincludes -g3 -fvisibility=hidden
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

clean:
			rm -f $(OBJS)

fclean:		clean
			rm -rf $(OBJS_DIR)
			rm -f $(NAME)
			rm -f $(LINK_NAME)

TEST_SRC	= tests/test_malloc.c
TEST_BIN	= run_tests

test:		all
			$(CC) -Wall -Wextra -Iincludes -DUSE_FT_MALLOC -o $(TEST_BIN) $(TEST_SRC) -L. -lft_malloc -lpthread
			DYLD_LIBRARY_PATH=. LD_LIBRARY_PATH=. ./$(TEST_BIN)

test_system:
			$(CC) -Wall -Wextra -Iincludes -o $(TEST_BIN) $(TEST_SRC) -lpthread
			./$(TEST_BIN)

re:			fclean all

.PHONY:		all clean fclean re test test_system