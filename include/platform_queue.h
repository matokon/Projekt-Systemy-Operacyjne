#pragma once
#include <sys/types.h>
#include "cablecar.h"

typedef struct {
    pid_t pid;
    int size;
} ped_group_t;

void platform_send_res(int qid, pid_t pid);
void platform_send_shutdown(int qid, pid_t pid);
void platform_flush_shutdown_waiters(int qid, pid_t *bikers, int *bikers_n,
                                     ped_group_t *peds, int *peds_n);
void platform_enqueue_request(int is_biker, pid_t pid, int group_size,
                              pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n, int max_queue);
void platform_try_form_groups(int qid, cablecar_t *cablecar, int sem_shm, int sem_chairs,
                              int sem_work_avail, pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n);
