CC=gcc
CFLAGS= -g -Wall -pedantic 
EJS = mrush
OBJECTS13_a = mining.o pow.o
#SEG_FAULT = -fsanitize=address -g3

all: $(EJS) clear

mrush: $(OBJECTS13_a)
	$(CC) $(CFLAGS) -o mrush $(OBJECTS13_a) -lpthread

mining.o: mining.c mining.h pow.h
	$(CC) $(CFLAGS) -c mining.c

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c pow.c

clear:
	rm -rf *.o

clean:
	rm -rf *.o $(EJS)

run:
	./mrush 0 20 21

runv:
	valgrind --leak-check=full --track-origins=yes ./mrush 200 5 24