#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/msg.h>
#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"
#include "platform_queue.h"

/* Flagi sterujące pauzą/stop_once pomiędzy emp1 i emp2. */
static volatile sig_atomic_t g_stopped = 0;
static volatile sig_atomic_t g_await_ack = 0;
static volatile sig_atomic_t g_initiator = 0;
static volatile sig_atomic_t g_sent_ack = 0;
static volatile sig_atomic_t g_other_pid = 0;

/* Czas MONOTONIC w mikrosekundach – używany do timeoutów drenażu. */
static long long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

/* Wysyłamy PLAT_SHUTDOWN bez blokowania kolejki (IPC_NOWAIT). */
static void send_shutdown_nowait(int qid, pid_t pid) {
    if (pid <= 0) return;
    platform_msg_t res;
    memset(&res, 0, sizeof(res));
    res.mtype = (long)pid;
    res.kind  = PLAT_SHUTDOWN;
    res.pid   = getpid();
    if (msgsnd(qid, &res, PLATFORM_MSGSZ, IPC_NOWAIT) == -1) {
        /* kolejka mogła zostać już usunięta lub być chwilowo pełna – ignorujemy */
    }
}

/* Po otrzymaniu PLAT_SHUTDOWN przez ~0.5s drenować kolejkę i odsyłać SHUTDOWN. */
static void drain_platform_shutdown(int qid, useconds_t duration_us) {
    const long long deadline = now_us() + duration_us;
    for (;;) {
        platform_msg_t late;
        ssize_t r = msgrcv(qid, &late, PLATFORM_MSGSZ, 0, IPC_NOWAIT);
        if (r >= 0) {
            send_shutdown_nowait(qid, late.pid);
            continue;
        }
        if (errno == EINTR) continue;
        if (errno != ENOMSG) break; /* inny błąd – przerywamy */
        if (now_us() >= deadline) break;
        usleep(5000);
    }
}

/* Handler SIGUSR1 – zatrzymanie pracy (pauza) przez emp2. */
static void on_sigusr1(int sig) {
    (void)sig;
    g_stopped = 1;
    g_initiator = 0;
    g_sent_ack = 0;
    printf(CLR_CYAN"    Pracownik1 %d: STOP (signal1)\n" RESET, getpid());
}

/* Handler SIGUSR2 – wznowienie pracy lub potwierdzenie stop_once. */
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

/* Rejestruje sygnały stop/wznowienie i zapisuje PID emp1 do shmem (chronione sem_shm). */
static void init_stop_signals(cablecar_t *cablecar, int sem_shm) {
    signal(SIGUSR1, on_sigusr1);
    signal(SIGUSR2, on_sigusr2);
    ipc_sem_wait(sem_shm);
    cablecar->emp1_pid = getpid();
    ipc_sem_post(sem_shm);
}

/* Czeka aż emp2 zapisze swój PID do shmem. */
static void wait_for_other_pid(cablecar_t *cablecar, int sem_shm) {
    /* Blokujące czekanie na PID emp2 w shmem – brak timeoutu i zbędnych logów. */
    for (;;) {
        ipc_sem_wait(sem_shm);
        pid_t other = cablecar->emp2_pid;
        ipc_sem_post(sem_shm);
        if (other > 0) {
            g_other_pid = other;
            return;
        }
        sched_yield();
    }
}

/* Jednorazowa pauza: po 5s pracy wysyła SIGUSR1 do emp2. */
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

/* Wznawia po pauzie stop_once po ~9s przerwy, wysyłając SIGUSR2 do emp2. */
static void maybe_resume(time_t stop_since) {
    if (!g_stopped || !g_initiator || g_await_ack) return;
    if (stop_since <= 0 || time(NULL) - stop_since < 9) return;
    g_await_ack = 1;
    g_sent_ack = 1;
    kill((pid_t)g_other_pid, SIGUSR2);
}

/* Obsługa peronu (emp1): odbiera PLAT_REQ, formuje grupy, obsługuje pauzy/PLAT_SHUTDOWN. */
int main() {
    printf(CLR_CYAN"    Pracownik1 Start: %d" RESET "\n", getpid());

    const char *s1 = getenv(IPC_ENV_PLATFORM_QID);
    if (!s1 || !*s1) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_PLATFORM_QID);
        return 1;
    }
    int platform_qid = (int)strtol(s1, NULL, 10);

    const char *s2 = getenv(IPC_ENV_SHM_CABLECAR);
    if (!s2 || !*s2) { 
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_SHM_CABLECAR);
        return 1;
     }
    int shmid = (int)strtol(s2, NULL, 10);
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    int sem_shm    = ipc_get_sem_from_env(IPC_ENV_SEM_SHM);
    int sem_chairs = ipc_get_sem_from_env(IPC_ENV_SEM_CHAIRS);
    int sem_gate4  = ipc_get_sem_from_env(IPC_ENV_SEM_GATE4);
    int sem_gate3  = ipc_get_sem_from_env(IPC_ENV_SEM_GATE3);
    int sem_inside = ipc_get_sem_from_env(IPC_ENV_SEM_INSIDE);
    int sem_exit2  = ipc_get_sem_from_env(IPC_ENV_SEM_EXIT2);

    init_stop_signals(cablecar, sem_shm);

    int stop_once = 0;
    const char *s3 = getenv(IPC_ENV_STOP_ONCE);
    if (s3 && *s3) stop_once = strtol(s3, NULL, 10) != 0;
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
            if (!closing) {
                closing = 1;
                printf(CLR_CYAN"    Pracownik1 %d: zamkniecie peronu" RESET "\n", getpid());
                /* Wybudz wszystkich oczekujacych na bramkach/semaforach. */
                for (int i = 0; i < 1024; i++) {
                    ipc_sem_post(sem_gate4);
                    ipc_sem_post(sem_gate3);
                    ipc_sem_post(sem_inside);
                    ipc_sem_post(sem_chairs);
                    ipc_sem_post(sem_exit2);
                }
                platform_flush_shutdown_waiters(platform_qid, bikers, &bikers_n, peds, &peds_n);
            }
            /* Dajemy ~0.5s na wyczyszczenie kolejki platformowej i odeslanie spoznionych. */
            drain_platform_shutdown(platform_qid, 500000);
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
