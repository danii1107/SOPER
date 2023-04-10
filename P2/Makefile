SRCS = principal.c candidato.c handlers.c votantes.c
OBJS = principal.o candidato.o handlers.o votantes.o
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pthread -lpthread
NAME = voting

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< 

clean:
	rm -f *.o

fclean: clean
	rm -f $(NAME)

re: fclean all