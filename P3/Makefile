CC = gcc
CFLAGS = -g -Wall -pedantic -lrt
EJS = monitor
OBJECTS= monitor.o
SEG_FAULT = -fsanitize=address -g3

all: $(EJS) clear

monitor: $(OBJECTS)
	$(CC) $(CFLAGS) $(SEG_FAULT) -o monitor $(OBJECTS)

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c monitor.c

clear:
	rm -rf *.o

clean:
	rm -rf *.o $(EJS)

c:
	./monitor 500

m: 
	./monitor 100

runv:
	valgrind --leak-check=full --track-origins=yes ./monitor 500
