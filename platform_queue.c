#include <string.h>
#include <stdio.h>
#include "platform_queue.h"
#include "ipc.h"

static void remove_ped_at(ped_group_t *peds, int *peds_n, int idx) {
    if (idx < 0 || idx >= *peds_n) return;
    if (idx < *peds_n - 1) {
        memmove(&peds[idx], &peds[idx + 1], (*peds_n - idx - 1) * sizeof(ped_group_t));
    }
    (*peds_n)--;
}

static int pick_peds_sum(ped_group_t *peds, int peds_n, int target,
                         int *idxs, int *idxs_n) {
    *idxs_n = 0;
    for (int i = 0; i < peds_n; i++) {
        if (peds[i].size == target) {
            idxs[0] = i;
            *idxs_n = 1;
            return 1;
        }
    }
    for (int i = 0; i < peds_n; i++) {
        for (int j = i + 1; j < peds_n; j++) {
            if (peds[i].size + peds[j].size == target) {
                idxs[0] = i;
                idxs[1] = j;
                *idxs_n = 2;
                return 1;
            }
        }
    }
    for (int i = 0; i < peds_n; i++) {
        for (int j = i + 1; j < peds_n; j++) {
            for (int k = j + 1; k < peds_n; k++) {
                if (peds[i].size + peds[j].size + peds[k].size == target) {
                    idxs[0] = i;
                    idxs[1] = j;
                    idxs[2] = k;
                    *idxs_n = 3;
                    return 1;
                }
            }
        }
    }
    for (int i = 0; i < peds_n; i++) {
        for (int j = i + 1; j < peds_n; j++) {
            for (int k = j + 1; k < peds_n; k++) {
                for (int l = k + 1; l < peds_n; l++) {
                    if (peds[i].size + peds[j].size + peds[k].size + peds[l].size == target) {
                        idxs[0] = i;
                        idxs[1] = j;
                        idxs[2] = k;
                        idxs[3] = l;
                        *idxs_n = 4;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

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
                                     ped_group_t *peds, int *peds_n) {
    for (int i = 0; i < *bikers_n; i++) platform_send_shutdown(qid, bikers[i]);
    for (int i = 0; i < *peds_n; i++) platform_send_shutdown(qid, peds[i].pid);
    *bikers_n = 0;
    *peds_n = 0;
}

void platform_enqueue_request(int is_biker, pid_t pid, int group_size,
                              pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n, int max_queue) {
    if (is_biker) {
        if (*bikers_n < max_queue) bikers[(*bikers_n)++] = pid;
    } else {
        if (group_size < 1) group_size = 1;
        if (*peds_n < max_queue) {
            peds[*peds_n].pid = pid;
            peds[*peds_n].size = group_size;
            (*peds_n)++;
        }
    }
}

void platform_try_form_groups(int qid, pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n) {
    for (;;) {
        if (*bikers_n >= 2) {
            pid_t p1 = bikers[0];
            pid_t p2 = bikers[1];
            memmove(&bikers[0], &bikers[2], (*bikers_n - 2) * sizeof(pid_t));
            *bikers_n -= 2;
            printf("PLATFORM GROUP: biker=%d biker=%d\n", (int)p1, (int)p2);
            platform_send_res(qid, p1);
            platform_send_res(qid, p2);
            continue;
        }
        if (*bikers_n >= 1) {
            int idxs[4];
            int idxs_n = 0;
            if (pick_peds_sum(peds, *peds_n, 2, idxs, &idxs_n)) {
                pid_t biker_pid = bikers[0];
                memmove(&bikers[0], &bikers[1], (*bikers_n - 1) * sizeof(pid_t));
                *bikers_n -= 1;
                printf("PLATFORM GROUP: biker=%d peds=", (int)biker_pid);
                for (int i = 0; i < idxs_n; i++) {
                    printf("%d(size=%d)%s",
                           (int)peds[idxs[i]].pid,
                           peds[idxs[i]].size,
                           (i + 1 == idxs_n) ? "" : ",");
                }
                printf("\n");
                platform_send_res(qid, biker_pid);
                for (int i = 0; i < idxs_n; i++) {
                    platform_send_res(qid, peds[idxs[i]].pid);
                }
                for (int i = idxs_n - 1; i >= 0; i--) {
                    remove_ped_at(peds, peds_n, idxs[i]);
                }
                continue;
            }
        }
        {
            int idxs[4];
            int idxs_n = 0;
            if (pick_peds_sum(peds, *peds_n, 4, idxs, &idxs_n)) {
                printf("PLATFORM GROUP: peds=");
                for (int i = 0; i < idxs_n; i++) {
                    printf("%d(size=%d)%s",
                           (int)peds[idxs[i]].pid,
                           peds[idxs[i]].size,
                           (i + 1 == idxs_n) ? "" : ",");
                }
                printf("\n");
                for (int i = 0; i < idxs_n; i++) {
                    platform_send_res(qid, peds[idxs[i]].pid);
                }
                for (int i = idxs_n - 1; i >= 0; i--) {
                    remove_ped_at(peds, peds_n, idxs[i]);
                }
                continue;
            }
        }
        break;
    }
}
