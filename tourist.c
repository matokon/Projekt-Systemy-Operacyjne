#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(void) {
    srand(time(NULL) ^ getpid());

    int czas = (rand() % 7) + 3;

    printf("    TURYSTA %d Start, będę żył %d s\n", getpid(), czas);
    sleep(czas);
    printf("\n    TURYSTA %d Koniec\n", getpid());
    return 0;
}
