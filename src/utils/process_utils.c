#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include "simulation.h"

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

pid_t* spawn_processes_for_seconds_collect(const char *path, const char *argv0,
                                          int duration_sec, int *out_count)
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
    time_t deadline = start + duration_sec;
    while (n1--) {
        if (time(NULL) >= deadline) break;

        int ms = (rand() % 900) + 100;
        // usleep((useconds_t)ms * 1000);

        if (time(NULL) - start >= duration_sec) break;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork spawn");
            break;
        }
        if (pid == 0) {
            execl(path, argv0, (char *)NULL);
            perror("execl spawn");
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

static void waitpid_eintr(pid_t pid) {
    int status;
    while (1) {
        pid_t r = waitpid(pid, &status, 0);
        if (r == -1 && errno == EINTR) continue;
        break;
    }
}

void wait_for_pids(pid_t *pids, int count) {
    for (int i = 0; i < count; i++) {
        waitpid_eintr(pids[i]);
    }
}

static volatile int g_child_threads_should_exit = 0;
static pthread_t *g_child_threads = NULL;
static int g_child_threads_count = 0;
static pthread_mutex_t g_child_mutex = PTHREAD_MUTEX_INITIALIZER;

void* child_thread_fn(void *arg) {
    (void)arg;
    /* Wątek dziecka symuluje obecność dziecka w grupie.
     * Kończy się gdy flaga g_child_threads_should_exit zostanie ustawiona
     * lub proces się kończy.
     */
    while (!g_child_threads_should_exit) {
        usleep(100000);  /* 100ms - sprawdzaj flagę co jakiś czas */
    }
    return NULL;
}

int spawn_child_thread(void) {
    pthread_t t;
    if (pthread_create(&t, NULL, child_thread_fn, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }
    /* Zapisz wątek do późniejszego zatrzymania */
    pthread_mutex_lock(&g_child_mutex);
    g_child_threads_count++;
    g_child_threads = realloc(g_child_threads, g_child_threads_count * sizeof(pthread_t));
    if (g_child_threads) {
        g_child_threads[g_child_threads_count - 1] = t;
    }
    pthread_mutex_unlock(&g_child_mutex);
    return 0;
}

void stop_child_threads(void) {
    pthread_mutex_lock(&g_child_mutex);
    g_child_threads_should_exit = 1;
    /* Czekaj na zakończenie wszystkich wątków dzieci */
    for (int i = 0; i < g_child_threads_count; i++) {
        pthread_join(g_child_threads[i], NULL);
    }
    free(g_child_threads);
    g_child_threads = NULL;
    g_child_threads_count = 0;
    pthread_mutex_unlock(&g_child_mutex);
}
