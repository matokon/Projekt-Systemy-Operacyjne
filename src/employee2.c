#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"

static volatile sig_atomic_t g_stop = 0;
static volatile sig_atomic_t g_stopped = 0;
static volatile sig_atomic_t g_await_ack = 0;
static volatile sig_atomic_t g_initiator = 0;
static volatile sig_atomic_t g_sent_ack = 0;
static volatile sig_atomic_t g_other_pid = 0;

static void handle_term(int sig) {
    (void)sig;
    g_stop = 1;
}

static void on_sigusr1(int sig) {
    (void)sig;
    g_stopped = 1;
    g_initiator = 0;
    g_sent_ack = 0;
    printf(CLR_BLUE"    Pracownik2 %d: STOP (signal1)\n" RESET, getpid());
}

static void on_sigusr2(int sig) {
    (void)sig;
    g_stopped = 0;
    printf(CLR_BLUE"    Pracownik2 %d: WZNOWIENIE (signal2)\n" RESET, getpid());
    if (g_await_ack) {
        g_await_ack = 0;
        return;
    }
    if (!g_sent_ack && g_other_pid > 0) {
        g_sent_ack = 1;
        kill((pid_t)g_other_pid, SIGUSR2);
    }
}

static void init_stop_signals(cablecar_t *cablecar, int sem_shm, int sem_emp2_ready) {
    signal(SIGTERM, handle_term);
    signal(SIGINT, handle_term);
    signal(SIGUSR1, on_sigusr1);
    signal(SIGUSR2, on_sigusr2);
    ipc_sem_wait(sem_shm);
    cablecar->emp2_pid = getpid();
    ipc_sem_post(sem_shm);
    ipc_sem_post(sem_emp2_ready);
}

static void wait_for_other_pid(cablecar_t *cablecar, int sem_shm, int sem_emp1_ready) {
    ipc_sem_wait(sem_emp1_ready);
    ipc_sem_wait(sem_shm);
    g_other_pid = cablecar->emp1_pid;
    ipc_sem_post(sem_shm);
    if (g_other_pid <= 0) {
        fprintf(stderr, "Pracownik2 %d: nie znaleziono PID pracownika1\n", getpid());
        exit(1);
    }
}

static void maybe_resume(time_t stop_since) {
    if (!g_stopped || !g_initiator || g_await_ack) return;
    if (stop_since <= 0 || time(NULL) - stop_since < 9) return;
    g_await_ack = 1;
    g_sent_ack = 1;
    kill((pid_t)g_other_pid, SIGUSR2);
}

int main() {
    printf(CLR_BLUE"    Pracownik2 Start: %d" RESET "\n", getpid());

    const char *s = getenv(IPC_ENV_SHM_CABLECAR);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_SHM_CABLECAR);
        return 1;
    }
    int shmid = (int)strtol(s, NULL, 10);
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    int sem_shm = ipc_get_sem_from_env(IPC_ENV_SEM_SHM);
    int sem_chairs = ipc_get_sem_from_env(IPC_ENV_SEM_CHAIRS);
    int sem_emp1_ready = ipc_get_sem_from_env(IPC_ENV_SEM_EMP1_READY);
    int sem_emp2_ready = ipc_get_sem_from_env(IPC_ENV_SEM_EMP2_READY);
    int sem_work_avail = ipc_get_sem_from_env(IPC_ENV_SEM_WORK_AVAIL);

    init_stop_signals(cablecar, sem_shm, sem_emp2_ready);
    wait_for_other_pid(cablecar, sem_shm, sem_emp1_ready);

    time_t stop_since = 0;
    for (; !g_stop;) {
        maybe_resume(stop_since);

        if (g_stopped) {
            sched_yield();
            continue;
        }
        
        int wait_result = ipc_sem_wait_interruptible(sem_work_avail);
        if (wait_result < 0) {
            if (g_stop) break;
            continue;
        }
        
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
