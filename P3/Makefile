CC = gcc
CFLAGS = -g -Wall -pedantic -lrt
EJS = monitor minero
OBJECTS_MON = monitor.o
OBJECTS_MIN = minero.o
#SEG_FAULT = -fsanitize=address -g3

all: $(EJS) clear

monitor: $(OBJECTS_MON)
	$(CC) $(CFLAGS) -o monitor $(OBJECTS_MON)

minero: $(OBJECTS_MIN)
	$(CC) $(CFLAGS) -o minero $(OBJECTS_MIN)

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c monitor.c

minero.o: minero.c minero.h
	$(CC) $(CFLAGS) -c minero.c

clear:
	rm -rf *.o

clean:
	rm -rf *.o $(EJS)

c:
	./monitor 500

m: 
	./monitor 100

p:
	./minero 8 100

runv:
	valgrind --leak-check=full --track-origins=yes ./monitor 500
