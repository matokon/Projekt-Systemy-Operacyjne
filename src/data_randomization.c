#include "ipc.h"

/* Losuje wiek z przedzialu 5-80 lat. */
int rand_age(void) {
    return (rand() % 76) + 5;
}

/* Zwraca 1 z prawdopodobienstwem 1%, inaczej 0. */
int rand_vip_1pct(void) {
    return (rand() % 100) == 0;
}

/* W 25% zwraca 0 (brak wyboru), inaczej losowy karnet PASS_SINGLE..PASS_DAY. */
pass_type_t rand_pass_or_zero(void) {
    if ((rand() % 4) == 0) return 0;
    return (pass_type_t)((rand() % 5) + 1);
}

/* Jesli req poprawny, zwraca go; inaczej losuje karnet 1..5. */
pass_type_t pick_pass(pass_type_t req) {
    if (req >= PASS_SINGLE && req <= PASS_DAY) return req;
    int r = (rand() % 5) + 1;
    return (pass_type_t)r;
}
