#include <string.h>
#include "platform_queue.h"
#include "ipc.h"

void platform_send_res(int qid, pid_t pid) {
    platform_msg_t res;
    memset(&res, 0, sizeof(res));
    res.mtype = (long)pid;
    res.kind = PLAT_RES;
    res.pid = pid;
    ipc_send_platform(qid, &res);
}

void platform_send_shutdown(int qid, pid_t pid) {
    platform_msg_t res;
    memset(&res, 0, sizeof(res));
    res.mtype = (long)pid;
    res.kind = PLAT_SHUTDOWN;
    res.pid = pid;
    ipc_send_platform(qid, &res);
}

void platform_flush_shutdown_waiters(int qid, pid_t *bikers, int *bikers_n,
                                     pid_t *peds, int *peds_n) {
    for (int i = 0; i < *bikers_n; i++) platform_send_shutdown(qid, bikers[i]);
    for (int i = 0; i < *peds_n; i++) platform_send_shutdown(qid, peds[i]);
    *bikers_n = 0;
    *peds_n = 0;
}

void platform_enqueue_request(int is_biker, pid_t pid, pid_t *bikers, int *bikers_n,
                              pid_t *peds, int *peds_n, int max_queue) {
    if (is_biker) {
        if (*bikers_n < max_queue) bikers[(*bikers_n)++] = pid;
    } else {
        if (*peds_n < max_queue) peds[(*peds_n)++] = pid;
    }
}

void platform_try_form_groups(int qid, pid_t *bikers, int *bikers_n,
                              pid_t *peds, int *peds_n) {
    for (;;) {
        if (*bikers_n >= 2) {
            pid_t p1 = bikers[0];
            pid_t p2 = bikers[1];
            memmove(&bikers[0], &bikers[2], (*bikers_n - 2) * sizeof(pid_t));
            *bikers_n -= 2;
            platform_send_res(qid, p1);
            platform_send_res(qid, p2);
            continue;
        }
        if (*bikers_n >= 1 && *peds_n >= 2) {
            pid_t p1 = bikers[0];
            pid_t p2 = peds[0];
            pid_t p3 = peds[1];
            memmove(&bikers[0], &bikers[1], (*bikers_n - 1) * sizeof(pid_t));
            *bikers_n -= 1;
            memmove(&peds[0], &peds[2], (*peds_n - 2) * sizeof(pid_t));
            *peds_n -= 2;
            platform_send_res(qid, p1);
            platform_send_res(qid, p2);
            platform_send_res(qid, p3);
            continue;
        }
        if (*peds_n >= 4) {
            pid_t p1 = peds[0];
            pid_t p2 = peds[1];
            pid_t p3 = peds[2];
            pid_t p4 = peds[3];
            memmove(&peds[0], &peds[4], (*peds_n - 4) * sizeof(pid_t));
            *peds_n -= 4;
            platform_send_res(qid, p1);
            platform_send_res(qid, p2);
            platform_send_res(qid, p3);
            platform_send_res(qid, p4);
            continue;
        }
        break;
    }
}
