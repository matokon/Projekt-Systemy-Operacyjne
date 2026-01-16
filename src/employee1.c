#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"
#include "sim_time.h"
#include "platform_queue.h"

static volatile sig_atomic_t g_stopped = 0;
static volatile sig_atomic_t g_await_ack = 0;
static volatile sig_atomic_t g_initiator = 0;
static volatile sig_atomic_t g_sent_ack = 0;
static volatile sig_atomic_t g_other_pid = 0;

static void on_sigusr1(int sig) {
    (void)sig;
    g_stopped = 1;
    g_initiator = 0;
    g_sent_ack = 0;
    printf(CLR_CYAN"    Pracownik1 %d: STOP (signal1)\n" RESET, getpid());
}

static void on_sigusr2(int sig) {
    (void)sig;
    g_stopped = 0;
    printf(CLR_CYAN"    Pracownik1 %d: WZNOWIENIE (signal2)\n" RESET, getpid());
    if (g_await_ack) {
        g_await_ack = 0;
        return;
    }
    if (!g_sent_ack && g_other_pid > 0) {
        g_sent_ack = 1;
        kill((pid_t)g_other_pid, SIGUSR2);
    }
}

static void init_stop_signals(cablecar_t *cablecar, int sem_shm) {
    signal(SIGUSR1, on_sigusr1);
    signal(SIGUSR2, on_sigusr2);
    ipc_sem_wait(sem_shm);
    cablecar->emp1_pid = getpid();
    ipc_sem_post(sem_shm);
}

static void wait_for_other_pid(cablecar_t *cablecar, int sem_shm) {
    for (;;) {
        ipc_sem_wait(sem_shm);
        pid_t other = cablecar->emp2_pid;
        ipc_sem_post(sem_shm);
        if (other > 0) {
            g_other_pid = other;
            break;
        }
        sleep(1);
    }
}

static void maybe_stop_once(int stop_once, int *stop_once_done,
                            time_t start_time, time_t *stop_since) {
    if (!stop_once || *stop_once_done || g_stopped) return;
    if (time(NULL) - start_time < 5) return;
    g_initiator = 1;
    g_stopped = 1;
    g_sent_ack = 0;
    *stop_since = time(NULL);
    kill((pid_t)g_other_pid, SIGUSR1);
    *stop_once_done = 1;
}

static void maybe_resume(time_t stop_since) {
    if (!g_stopped || !g_initiator || g_await_ack) return;
    if (stop_since <= 0 || time(NULL) - stop_since < 9) return;
    g_await_ack = 1;
    g_sent_ack = 1;
    kill((pid_t)g_other_pid, SIGUSR2);
}

int main() {
    printf(CLR_CYAN"    Pracownik1 Start: %d" RESET "\n", getpid());
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    const char *s1 = getenv(IPC_ENV_PLATFORM_QID);
    if (!s1 || !*s1) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_PLATFORM_QID);
        return 1;
    }
    int platform_qid = atoi(s1);

    const char *s2 = getenv(IPC_ENV_SHM_CABLECAR);
    if (!s2 || !*s2) { 
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_SHM_CABLECAR);
        return 1;
     }
    int shmid = atoi(s2);
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    int sem_shm = ipc_get_sem_from_env(IPC_ENV_SEM_SHM);
    int sem_chairs = ipc_get_sem_from_env(IPC_ENV_SEM_CHAIRS);

    init_stop_signals(cablecar, sem_shm);

    int stop_once = 0;
    const char *s3 = getenv(IPC_ENV_STOP_ONCE);
    if (s3 && *s3) stop_once = atoi(s3) != 0;
    int stop_once_done = 0;
    time_t start_time = time(NULL);

    wait_for_other_pid(cablecar, sem_shm);


    enum { MAX_QUEUE = 1024 };
    pid_t bikers[MAX_QUEUE];
    ped_group_t peds[MAX_QUEUE];
    int bikers_n = 0;
    int peds_n = 0;
    int closing = 0;
    int gate_closed = 0;
    time_t stop_since = 0;

    for (;;) {
        platform_msg_t req;
        memset(&req, 0, sizeof(req));
        if (ipc_recv_platform(platform_qid, -MT_NORMAL, &req, 0) < 0) break;

        maybe_stop_once(stop_once, &stop_once_done, start_time, &stop_since);
        maybe_resume(stop_since);

        if (req.kind == PLAT_SHUTDOWN) {
            if (closing) {
                if (req.pid > 0) {
                    platform_msg_t ack;
                    memset(&ack, 0, sizeof(ack));
                    ack.mtype = (long)req.pid;
                    ack.kind = PLAT_SHUTDOWN_ACK;
                    ack.pid = getpid();
                    ipc_send_platform(platform_qid, &ack);
                }
                printf(CLR_CYAN"    Pracownik1 %d: shutdown final" RESET "\n", getpid());
                break;
            }
            closing = 1;
            printf(CLR_CYAN"    Pracownik1 %d: zamkniecie peronu" RESET "\n", getpid());
            platform_flush_shutdown_waiters(platform_qid, bikers, &bikers_n, peds, &peds_n);
            continue;
        }
        if (req.kind != PLAT_REQ || req.pid <= 0) {
            continue;
        }

        if (closing) {
            platform_send_shutdown(platform_qid, req.pid);
            continue;
        }

        if (sim_is_closed()) {
            if (!gate_closed) {
                printf(CLR_CYAN"    Pracownik1 %d: bramki peronu zamkniete (po Tk)\n" RESET, getpid());
                gate_closed = 1;
            }
            platform_send_shutdown(platform_qid, req.pid);
            continue;
        }

        platform_enqueue_request(req.is_biker, req.pid, req.group_size,
                                 bikers, &bikers_n, peds, &peds_n, MAX_QUEUE);
        if (!g_stopped) {
            platform_try_form_groups(platform_qid, cablecar, sem_shm, sem_chairs,
                                     bikers, &bikers_n, peds, &peds_n);
        }
    }

    printf(CLR_CYAN"    Pracownik1 Koniec: %d" RESET "\n", getpid());
    return 0;
}
