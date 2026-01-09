#pragma once
#include "cablecar.h"

void set_env_int(const char *name, int value);
void set_env_sems(int sem_gate4, int sem_gate3, int sem_inside, int sem_chairs, int sem_shm);
void cleanup_ipc(int qid, int platform_qid,
                 int sem_gate4, int sem_gate3, int sem_inside,
                 int sem_chairs, int sem_shm,
                 cablecar_t *cablecar, int shmid);
