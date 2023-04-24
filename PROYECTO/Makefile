CC = gcc
CFLAGS = -g -Wall -pedantic -lrt
EJS = monitor minero
OBJECTS_MON = monitor.o pow.o
OBJECTS_MIN = minero.o pow.o
#SEG_FAULT = -fsanitize=address -g3

all: $(EJS) clear

monitor: $(OBJECTS_MON)
	$(CC) $(CFLAGS) -o monitor $(OBJECTS_MON)

minero: $(OBJECTS_MIN)
	$(CC) $(CFLAGS) -o minero $(OBJECTS_MIN)

monitor.o: monitor.c monitor.h pow.h 
	$(CC) $(CFLAGS) -c monitor.c

minero.o: minero.c minero.h pow.h
	$(CC) $(CFLAGS) -c minero.c

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c pow.c

clear:
	rm -rf *.o

clean:
	rm -rf *.o $(EJS)