#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "main_utils.h"
#include "ipc.h"
#include "cablecar.h"

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
                 int sem_q_guard, int sem_pq_guard, int sem_emp_ready,
                 cablecar_t *cablecar, int shmid) {
    ipc_destroy_queue(qid);
    ipc_destroy_queue(platform_qid);
    ipc_destroy_sem(sem_gate4);
    ipc_destroy_sem(sem_gate3);
    ipc_destroy_sem(sem_inside);
    ipc_destroy_sem(sem_chairs);
    ipc_destroy_sem(sem_q_guard);
    ipc_destroy_sem(sem_pq_guard);
    ipc_destroy_sem(sem_emp_ready);
    ipc_destroy_sem(sem_exit2);
    ipc_detach_shm(cablecar);
    ipc_destroy_shm(shmid);
    ipc_destroy_sem(sem_shm);
}

int generate_report(const char *log_path, const char *out_path) {
    FILE *in = fopen(log_path, "r");
    if (!in) {
        fprintf(stderr, "report: cannot open %s: %s\n", log_path, strerror(errno));
        return -1;
    }

    unsigned int *counts = NULL;
    size_t counts_sz = 0;
    char line[256];

    while (fgets(line, sizeof(line), in)) {
        unsigned int pass_id = 0;
        char ts[32] = {0};
        char gate[16] = {0};
        if (sscanf(line, "%u;%31[^;];gate=%15s", &pass_id, ts, gate) != 3) continue;
        if (pass_id == 0) continue;
        if (strcmp(gate, "platform") != 0) continue;
        if (pass_id >= counts_sz) {
            size_t new_sz = pass_id + 1;
            unsigned int *tmp = (unsigned int*)realloc(counts, new_sz * sizeof(*counts));
            if (!tmp) {
                fclose(in);
                free(counts);
                perror("report realloc");
                return -1;
            }
            for (size_t i = counts_sz; i < new_sz; i++) tmp[i] = 0;
            counts = tmp;
            counts_sz = new_sz;
        }
        counts[pass_id]++;
    }
    fclose(in);

    FILE *out = fopen(out_path, "a");
    if (!out) {
        fprintf(stderr, "report: cannot open %s: %s\n", out_path, strerror(errno));
        free(counts);
        return -1;
    }
    fprintf(out, "\n--- PODSUMOWANIE PRZEJAZDOW ---\n");
    for (size_t i = 1; i < counts_sz; i++) {
        if (counts[i]) fprintf(out, "%zu;%u\n", i, counts[i]);
    }
    fclose(out);
    free(counts);
    return 0;
}

void wait_for_cablecar_empty(cablecar_t *cablecar, int sem_shm) {
    for (;;) {
        int occ = 0;
        ipc_sem_wait(sem_shm);
        occ = cablecar->occupied;
        ipc_sem_post(sem_shm);
        if (occ == 0) break;
        // sleep(1);
    }
}

int parse_arg_int(const char *s) {
    long v = strtol(s, NULL, 10);
    return (int)v;
}

void cablecar_init(cablecar_t *cablecar)
{
    cablecar->head = 0;
    cablecar->tail = 0;
    cablecar->occupied = 0;
    cablecar->vip_waiting = 0;

    for (int i = 0; i < 72; i++) {
        cablecar->seats[i].state = SEAT_FREE;
        cablecar->seats[i].group_size = 0;
        for(int j = 0; j < 4; j++)
        {
            cablecar->seats[i].pids[j] = 0;
        }
    }
    cablecar->emp1_pid = 0;
    cablecar->emp2_pid = 0;
}
