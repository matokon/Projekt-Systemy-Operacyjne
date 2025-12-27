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

    while (1) {
        if (time(NULL) - start >= duration_sec) break;

        int ms = (rand() % 900) + 100;
        usleep((useconds_t)ms * 1000);

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