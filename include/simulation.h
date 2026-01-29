#pragma once
#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "cablecar.h"

#define CLR_BLUE   "\x1b[34m"
#define CLR_CYAN   "\x1b[36m"
#define CLR_GREEN  "\x1b[32m"
#define CLR_YELLOW "\x1b[33m"
#define CLR_PINK   "\x1b[1;95m"
#define CLR_RED_B  "\x1b[1;31m"
#define RESET "\x1B[0m"

pid_t start_process(const char *path, const char *argv0, const char *msg);
pid_t* spawn_processes_for_seconds_collect(const char *path, const char *argv0,
                                          int duration_sec, int sem_tourists, int *out_count);

void wait_for_pids(pid_t *pids, int count);
void* child_thread_fn(void *arg);
int spawn_child_thread(void);

static inline int sim_get_tp(void) {
    const char *s = getenv(IPC_ENV_TP);
    if (!s || !*s) return 0;
    return (int)strtol(s, NULL, 10);
}

static inline int sim_get_tk(void) {
    const char *s = getenv(IPC_ENV_TK);
    if (!s || !*s) return 0;
    return (int)strtol(s, NULL, 10);
}

static inline int sim_get_start(void) {
    const char *s = getenv(IPC_ENV_START);
    if (!s || !*s) return 0;
    return (int)strtol(s, NULL, 10);
}

static inline int sim_now(void) {
    int tp = sim_get_tp();
    int start = sim_get_start();
    time_t now = time(NULL);
    int delta = (int)(now - (time_t)start);
    if (delta < 0) delta = 0;
    return tp + delta;
}

static inline int sim_is_closed(void) {
    return sim_now() >= sim_get_tk();
}

#endif
