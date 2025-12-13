#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include "simulation.h"

void  start_process(const char *path, const char *name, const char *msg)
{
    pid_t process_pid = fork();

    if(process_pid == -1)
    {
        perror(msg);
        exit(1);
    }
    else if(process_pid == 0)
    {
        execl(path, name, (char *)NULL);
        perror("execl error");
        _exit(1);
    }
}

int spawn_processes_for_seconds(const char *path, const char *argv0, int duration_sec)
{
    time_t start = time(NULL);
    int spawned = 0;

    while (1) {
        if (time(NULL) - start >= duration_sec) break;

        int ms = (rand() % 900) + 100;
        usleep((useconds_t)ms * 1000);

        if (time(NULL) - start >= duration_sec) break;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork spawn_processes_for_seconds");
            break;
        }
        if (pid == 0) {
            execl(path, argv0, (char *)NULL);
            perror("execl spawn_processes_for_seconds");
            _exit(1);
        }
        spawned++;
    }
    return spawned;
}
