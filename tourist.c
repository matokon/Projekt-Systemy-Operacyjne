#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "simulation.h"

int main(void) {
    srand(time(NULL) ^ getpid());

    int czas = (rand() % 7) + 3;

    printf(CLR_GREEN"    TURYSTA %d Start, będę żył %d s" RESET "\n", getpid(), czas);
    sleep(czas);
    printf(CLR_GREEN"\n    TURYSTA %d Koniec" RESET "\n", getpid());
    return 0;
}
