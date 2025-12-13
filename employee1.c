#include <stdio.h>
#include <unistd.h>
#include "simulation.h"

int main()
{
    printf(CLR_CYAN"    Pracownik1 Start: %d" RESET "\n", getpid());
    sleep(12);
    printf(CLR_CYAN"    Pracownik1 Koniec: %d" RESET "\n", getpid());
    return 0;
}