CC = gcc
CFLAGS = -g -Wall -pedantic 
EJS = monitor
OBJECTS= monitor.o
#SEG_FAULT = -fsanitize=address -g3

all: $(EJS) clear

monitor: $(OBJECTS)
	$(CC) $(CFLAGS) -o monitor $(OBJECTS) -lrt

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c monitor.c

clear:
	rm -rf *.o

clean:
	rm -rf *.o $(EJS)

run:
	./monitor 500

runv:
	valgrind --leak-check=full --track-origins=yes ./monitor 500