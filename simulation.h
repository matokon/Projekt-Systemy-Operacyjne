#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>



void start_process(const char *path, const char *name, const char *msg);
int spawn_processes_for_seconds(const char *path, const char *argv0, int duration_sec);
#endif
