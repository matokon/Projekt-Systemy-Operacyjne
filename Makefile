CC     = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

OBJS = main.o process_utils.o ipc.o cashier.o data_randomization.o \
       employee1.o platform_queue.o employee2.o tourist.o tourist_utils.o
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: projekt cashier employee1 employee2 tourist

# --------- projekt (main) ---------
projekt: main.o process_utils.o ipc.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: src/main.c include/simulation.h include/ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

data_randomization.o: src/data_randomization.c include/simulation.h include/ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

# --------- wspólne utils ---------
process_utils.o: src/process_utils.c include/simulation.h
	$(CC) $(CFLAGS) -c $< -o $@

# --------- ipc ---------
ipc.o: src/ipc.c include/ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

# --------- cashier / employee / tourist ---------
cashier: cashier.o ipc.o data_randomization.o
	$(CC) $(CFLAGS) -o $@ $^

cashier.o: src/cashier.c include/simulation.h include/ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

employee1: employee1.o ipc.o platform_queue.o
	$(CC) $(CFLAGS) -o $@ $^

employee1.o: src/employee1.c include/simulation.h include/ipc.h include/platform_queue.h
	$(CC) $(CFLAGS) -c $< -o $@

platform_queue.o: src/platform_queue.c include/platform_queue.h include/ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

employee2: employee2.o
	$(CC) $(CFLAGS) -o $@ $^

employee2.o: src/employee2.c include/simulation.h
	$(CC) $(CFLAGS) -c $< -o $@

tourist: tourist.o tourist_utils.o ipc.o data_randomization.o process_utils.o
	$(CC) $(CFLAGS) -o $@ $^

tourist.o: src/tourist.c include/simulation.h include/ipc.h include/tourist_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

tourist_utils.o: src/tourist_utils.c include/tourist_utils.h include/simulation.h include/ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f projekt cashier employee1 employee2 tourist *.o *.d
