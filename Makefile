CC     = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: projekt cashier employee1 employee2 tourist

# GŁÓWNY PROGRAM (rodzic)
projekt: main.o utils.o
	$(CC) $(CFLAGS) -o projekt main.o utils.o

main.o: main.c simulation.h
	$(CC) $(CFLAGS) -c main.c -o main.o

utils.o: utils.c simulation.h
	$(CC) $(CFLAGS) -c utils.c -o utils.o

cashier: cashier.c
	$(CC) $(CFLAGS) cashier.c -o cashier

employee1: employee1.c
	$(CC) $(CFLAGS) employee1.c -o employee1

employee2: employee2.c
	$(CC) $(CFLAGS) employee2.c -o employee2

tourist: tourist.c
	$(CC) $(CFLAGS) tourist.c -o tourist

ipc.o: ipc.c ipc.h
	$(CC) $(CFLAGS) -c ipc.c -o ipc.o

clean:
	rm -f projekt cashier employee1 employee2 tourist *.o
