#pragma once
#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define CLR_BLUE   "\x1b[34m"
#define CLR_CYAN   "\x1b[36m"
#define CLR_GREEN  "\x1b[32m"
#define CLR_YELLOW "\x1b[33m"
#define CLR_PINK   "\x1b[1;95m"
#define RESET "\x1B[0m"

pid_t start_process(const char *path, const char *argv0, const char *msg);
pid_t* spawn_processes_for_seconds_collect(const char *path, const char *argv0,
                                          int duration_sec, int *out_count);

void wait_for_pids(pid_t *pids, int count);

#endif
