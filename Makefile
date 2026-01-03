CC     = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: projekt cashier employee1 employee2 tourist

# --------- projekt (main) ---------
projekt: main.o process_utils.o ipc.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c simulation.h ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

data_randomization.o: data_randomization.c simulation.h ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

# --------- wspólne utils ---------
process_utils.o: process_utils.c simulation.h
	$(CC) $(CFLAGS) -c $< -o $@

# --------- ipc ---------
ipc.o: ipc.c ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

# --------- cashier / employee / tourist ---------
cashier: cashier.o ipc.o data_randomization.o
	$(CC) $(CFLAGS) -o $@ $^

cashier.o: cashier.c simulation.h ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

employee1: employee1.o ipc.o platform_queue.o
	$(CC) $(CFLAGS) -o $@ $^

employee1.o: employee1.c simulation.h ipc.h platform_queue.h
	$(CC) $(CFLAGS) -c $< -o $@

platform_queue.o: platform_queue.c platform_queue.h ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

employee2: employee2.o
	$(CC) $(CFLAGS) -o $@ $^

employee2.o: employee2.c simulation.h
	$(CC) $(CFLAGS) -c $< -o $@

tourist: tourist.o tourist_utils.o ipc.o data_randomization.o process_utils.o
	$(CC) $(CFLAGS) -o $@ $^

tourist.o: tourist.c simulation.h ipc.h tourist_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

tourist_utils.o: tourist_utils.c tourist_utils.h simulation.h ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f projekt cashier employee1 employee2 tourist *.o
