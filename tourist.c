#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "simulation.h"

int main(void) {
    srand(time(NULL) ^ getpid());

    int time_of_life = (rand() % 7) + 3;

    printf(CLR_GREEN"    TURYSTA %d Start, będę żył %d s" RESET "\n", getpid(), time_of_life);
    sleep(time_of_life);
    printf(CLR_GREEN"\n    TURYSTA %d Koniec" RESET "\n", getpid());
    return 0;
}
