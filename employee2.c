#include <stdio.h>
#include <unistd.h>
#include "simulation.h"

int main()
{
    printf(CLR_BLUE"    Pracownik2 Start: %d" RESET "\n", getpid());
    sleep(14);
    printf(CLR_BLUE"    Pracownik2 Koniec: %d" RESET "\n", getpid());
    return 0;
}