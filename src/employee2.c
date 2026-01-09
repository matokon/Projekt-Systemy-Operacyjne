#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"

static volatile sig_atomic_t g_stop = 0;

static void handle_term(int sig) {
    (void)sig;
    g_stop = 1;
}

int main() {
    printf(CLR_BLUE"    Pracownik2 Start: %d" RESET "\n", getpid());

    const char *s = getenv(IPC_ENV_SHM_CABLECAR);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_SHM_CABLECAR);
        return 1;
    }
    int shmid = atoi(s);
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    int sem_shm = ipc_get_sem_from_env(IPC_ENV_SEM_SHM);
    int sem_chairs = ipc_get_sem_from_env(IPC_ENV_SEM_CHAIRS);

    signal(SIGTERM, handle_term);
    signal(SIGINT, handle_term);

    for (; !g_stop;) {
        sleep(2);
        ipc_sem_wait(sem_shm);
        if (cablecar->occupied > 0 && cablecar->seats[cablecar->head].state == SEAT_OCCUPIED) {
            seat_t *seat = &cablecar->seats[cablecar->head];
            seat->state = SEAT_FREE;
            seat->group_size = 0;
            for (int i = 0; i < 4; i++) seat->pids[i] = 0;
            cablecar->head = (cablecar->head + 1) % 72;
            cablecar->occupied--;
            ipc_sem_post(sem_chairs);
        }
        ipc_sem_post(sem_shm);
    }

    ipc_detach_shm(cablecar);
    printf(CLR_BLUE"    Pracownik2 Koniec: %d" RESET "\n", getpid());
    return 0;
}
