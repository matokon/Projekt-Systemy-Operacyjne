#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

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