#pragma once
#include "cablecar.h"

void set_env_int(const char *name, int value);
void set_env_sems(int sem_gate4, int sem_gate3, int sem_inside,
                  int sem_chairs, int sem_shm, int sem_exit2);
void cleanup_ipc(int qid, int platform_qid,
                 int sem_gate4, int sem_gate3, int sem_inside,
                 int sem_chairs, int sem_shm, int sem_exit2,
                 int sem_emp1_ready, int sem_emp2_ready, int sem_work_avail,
                 cablecar_t *cablecar, int shmid);
int generate_report(const char *log_path, const char *out_path);
void wait_for_cablecar_empty(cablecar_t *cablecar, int sem_shm, int sem_work_avail);
int parse_arg_int(const char *s);
