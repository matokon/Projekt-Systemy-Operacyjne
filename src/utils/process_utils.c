#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include "simulation.h"
#include "ipc.h"

/* Fork+exec procesu potomnego, z komunikatem błędu gdy nie powiedzie się exec. */
pid_t start_process(const char *path, const char *argv0, const char *msg)
{
    pid_t pid = fork();
    if(pid == -1) {
        perror(msg);
        exit(1);
    } else if(pid == 0) {
        execl(path, argv0, (char *)NULL);
        perror("execl error");
        _exit(1);
    }
    return pid;
}

/* Przez zadany czas forkuje procesy child, kontrolując liczbę aktywnych kredytem sem_tourists. */
pid_t* spawn_processes_for_seconds_collect(const char *path, const char *argv0,
                                          int duration_sec, int sem_tourists, int *out_count)
{
    time_t start = time(NULL);

    int cap = 64;
    int n = 0;
    pid_t *pids = (pid_t*)malloc(sizeof(pid_t) * cap);
    if (!pids) {
        perror("malloc pids");
        exit(1);
    }
    int n1 = 5000;
    while (n1--) {
        if (time(NULL) - start >= duration_sec) break;

        /* Ograniczenie liczby aktywnych turystów tokenami semafora. */
        if (ipc_sem_wait(sem_tourists) < 0) break;

        if (time(NULL) - start >= duration_sec) {
            ipc_sem_post(sem_tourists);
            break;
        }

        if (time(NULL) - start >= duration_sec) break;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork spawn");
            ipc_sem_post(sem_tourists);
            break;
        }
        if (pid == 0) {
            execl(path, argv0, (char *)NULL);
            perror("execl spawn");
            ipc_sem_post(sem_tourists);
            _exit(1);
        }

        if (n == cap) {
            cap *= 2;
            pid_t *np = (pid_t*)realloc(pids, sizeof(pid_t) * cap);
            if (!np) {
                perror("realloc pids");
                free(pids);
                exit(1);
            }
            pids = np;
        }
        pids[n++] = pid;
    }

    *out_count = n;
    return pids;
}

/* waitpid odporne na EINTR. */
static void waitpid_eintr(pid_t pid) {
    int status;
    while (1) {
        pid_t r = waitpid(pid, &status, 0);
        if (r == -1 && errno == EINTR) continue;
        break;
    }
}

/* Blokująco czeka na wszystkie PIDy z tablicy. */
void wait_for_pids(pid_t *pids, int count) {
    for (int i = 0; i < count; i++) {
        waitpid_eintr(pids[i]);
    }
}

/* Wątek "dziecka" – sztuczne obciążenie, pętla nieskończona. */
void* child_thread_fn(void *arg) {
    (void)arg;
    for (;;) {
        // sleep(3600);
    }
    return NULL;
}

/* Startuje odłączony wątek reprezentujący dziecko turysty. */
int spawn_child_thread(void) {
    pthread_t t;
    if (pthread_create(&t, NULL, child_thread_fn, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }
    pthread_detach(t);
    return 0;
}
