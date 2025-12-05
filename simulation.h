#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>



void start_process(const char *path, const char *name, const char *msg);

#endif
