#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cablecar.h"
#include "sim_time.h"

static int get_env_int(const char *name) {
    const char *s = getenv(name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", name);
        exit(1);
    }
    return atoi(s);
}

int sim_get_tp(void) {
    return get_env_int(IPC_ENV_TP);
}

int sim_get_tk(void) {
    return get_env_int(IPC_ENV_TK);
}

int sim_get_start(void) {
    return get_env_int(IPC_ENV_START);
}

int sim_now(void) {
    int tp = sim_get_tp();
    int start = sim_get_start();
    time_t now = time(NULL);
    int delta = (int)(now - (time_t)start);
    if (delta < 0) delta = 0;
    return tp + delta;
}

int sim_is_closed(void) {
    return sim_now() >= sim_get_tk();
}
