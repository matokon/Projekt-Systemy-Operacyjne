#pragma once
#include <sys/types.h>

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
void platform_try_form_groups(int qid, pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n);
