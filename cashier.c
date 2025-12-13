#include <stdio.h>
#include <unistd.h>
#include "simulation.h"

int main()
{
    printf(CLR_YELLOW"    Kasjer Start: %d" RESET "\n", getpid());
    sleep(10);
    printf(CLR_YELLOW"    Kasjer Koniec: %d" RESET "\n", getpid());
    return 0;
}