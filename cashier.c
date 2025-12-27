#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/msg.h>

#include "simulation.h"
#include "ipc.h"

int main()
{
    printf(CLR_YELLOW"    Kasjer Start: %d" RESET "\n", getpid());
    sleep(10);
    printf(CLR_YELLOW"    Kasjer Koniec: %d" RESET "\n", getpid());
    return 0;
}