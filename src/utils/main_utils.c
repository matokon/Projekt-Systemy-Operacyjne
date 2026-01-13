#include <stdio.h>
#include <stdlib.h>
#include "main_utils.h"
#include "ipc.h"

void set_env_int(const char *name, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    if (setenv(name, buf, 1) != 0) {
        perror("setenv");
        exit(1);
    }
}

void set_env_sems(int sem_gate4, int sem_gate3, int sem_inside,
                  int sem_chairs, int sem_shm, int sem_exit2) {
    ipc_set_env_sem(IPC_ENV_SEM_GATE4, sem_gate4);
    ipc_set_env_sem(IPC_ENV_SEM_GATE3, sem_gate3);
    ipc_set_env_sem(IPC_ENV_SEM_INSIDE, sem_inside);
    ipc_set_env_sem(IPC_ENV_SEM_CHAIRS, sem_chairs);
    ipc_set_env_sem(IPC_ENV_SEM_SHM, sem_shm);
    ipc_set_env_sem(IPC_ENV_SEM_EXIT2, sem_exit2);
}

void cleanup_ipc(int qid, int platform_qid,
                 int sem_gate4, int sem_gate3, int sem_inside,
                 int sem_chairs, int sem_shm, int sem_exit2,
                 cablecar_t *cablecar, int shmid) {
    ipc_destroy_queue(qid);
    ipc_destroy_queue(platform_qid);
    ipc_destroy_sem(sem_gate4);
    ipc_destroy_sem(sem_gate3);
    ipc_destroy_sem(sem_inside);
    ipc_destroy_sem(sem_chairs);
    ipc_destroy_sem(sem_exit2);
    ipc_detach_shm(cablecar);
    ipc_destroy_shm(shmid);
    ipc_destroy_sem(sem_shm);
}
