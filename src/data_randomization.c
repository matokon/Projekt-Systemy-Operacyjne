#include "ipc.h"

/* Losuje wiek 5-80 lat. */
int rand_age(void) {
    return (rand() % 76) + 5;
}

/* Zwraca 1 z prawdopodobieństwem ~1%. */
int rand_vip_1pct(void) {
    return (rand() % 100) == 0;
}

/* Losuje rodzaj karnetu lub 0 (brak preferencji) z szansą 1/4 na zero. */
pass_type_t rand_pass_or_zero(void) {
    if ((rand() % 4) == 0) return 0;
    return (pass_type_t)((rand() % 5) + 1);
}

/* Wybiera faktyczny rodzaj karnetu; gdy req=0 losuje PASS_* (1..5). */
pass_type_t pick_pass(pass_type_t req) {
    if (req >= PASS_SINGLE && req <= PASS_DAY) return req;
    int r = (rand() % 5) + 1;
    return (pass_type_t)r;
}

