NAME		= libft_malloc.so

SRCS		= srcs/malloc.c \
			  srcs/free.c \
			  srcs/realloc.c 

OBJS_DIR	= objs
OBJS		= $(SRCS:srcs/%.c=$(OBJS_DIR)/%.o)

CC			= gcc
CFLAGS		= -Wall -Wextra -Werror -fPIC -Iincludes
LDFLAGS		= -shared

all:		$(OBJS_DIR) $(NAME)

$(NAME):	$(OBJS)
			$(CC) $(LDFLAGS) -o $(NAME) $(OBJS)

$(OBJS_DIR):
			mkdir -p $(OBJS_DIR)

$(OBJS_DIR)/%.o:	srcs/%.c
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			rm -f $(OBJS)

fclean:		clean
			rm -rf $(OBJS_DIR)
			rm -f $(NAME)

re:			fclean all

.PHONY:		all clean fclean re