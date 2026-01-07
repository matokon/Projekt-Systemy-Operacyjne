#include "ipc.h"

int rand_age(void) {
    return (rand() % 76) + 5;
}

int rand_vip_1pct(void) {
    return (rand() % 100) == 0;
}

pass_type_t rand_pass_or_zero(void) {
    if ((rand() % 4) == 0) return 0;
    return (pass_type_t)((rand() % 5) + 1);
}

pass_type_t pick_pass(pass_type_t req) {
    if (req >= PASS_SINGLE && req <= PASS_DAY) return req;
    int r = (rand() % 5) + 1;
    return (pass_type_t)r;
}

