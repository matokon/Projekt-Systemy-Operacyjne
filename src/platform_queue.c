#include <string.h>
#include "platform_queue.h"
#include "ipc.h"
#include "cablecar.h"

/* Usuwa piesza grupe z tablicy pod wskazanym indeksem. */
static void remove_ped_at(ped_group_t *peds, int *peds_n, int idx) {
    if (idx < 0 || idx >= *peds_n) return;
    if (idx < *peds_n - 1) {
        memmove(&peds[idx], &peds[idx + 1], (*peds_n - idx - 1) * sizeof(ped_group_t));
    }
    (*peds_n)--;
}

/* Szuka kombinacji grup pieszych o sumie target; zapisuje indeksy w idxs. */
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

/* Buduje i wysyla komunikat PLAT_RES do procesu pid. */
void platform_send_res(int qid, pid_t pid) {
    platform_msg_t res;
    memset(&res, 0, sizeof(res));
    res.mtype = (long)pid;
    res.kind = PLAT_RES;
    res.pid = pid;
    ipc_send_platform(qid, &res);
}

/* Wysyla PLAT_SHUTDOWN do procesu pid. */
void platform_send_shutdown(int qid, pid_t pid) {
    platform_msg_t res;
    memset(&res, 0, sizeof(res));
    res.mtype = (long)pid;
    res.kind = PLAT_SHUTDOWN;
    res.pid = pid;
    ipc_send_platform(qid, &res);
}

/* W fazie zamykania wysyla PLAT_SHUTDOWN do wszystkich oczekujacych. */
void platform_flush_shutdown_waiters(int qid, pid_t *bikers, int *bikers_n,
                                     ped_group_t *peds, int *peds_n) {
    for (int i = 0; i < *bikers_n; i++) platform_send_shutdown(qid, bikers[i]);
    for (int i = 0; i < *peds_n; i++) platform_send_shutdown(qid, peds[i].pid);
    *bikers_n = 0;
    *peds_n = 0;
}

/* Dodaje zgłoszenie na peron do kolejki rowerzystow lub pieszych w limicie max_queue. */
void platform_enqueue_request(int is_biker, pid_t pid, int group_size,
                              pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n, int max_queue) {
    if (is_biker) {
        if (*bikers_n < max_queue) bikers[(*bikers_n)++] = pid;
    } else {
        if (*peds_n < max_queue) {
            peds[*peds_n].pid = pid;
            peds[*peds_n].size = group_size;
            (*peds_n)++;
        }
    }
}

/* Rezerwuje miejsce w wagoniku: oznacza siedzenie jako zajete i wpisuje PIDy. */
static void reserve_seat(cablecar_t *cablecar, int sem_shm, int sem_chairs,
                         pid_t *pids, int pids_n, int group_size) {
    ipc_sem_wait(sem_chairs);
    ipc_sem_wait(sem_shm);
    seat_t *seat = &cablecar->seats[cablecar->tail];
    seat->state = SEAT_OCCUPIED;
    seat->group_size = group_size;
    for (int i = 0; i < 4; i++) seat->pids[i] = 0;
    for (int i = 0; i < pids_n && i < 4; i++) seat->pids[i] = pids[i];
    cablecar->tail = (cablecar->tail + 1) % 72;
    cablecar->occupied++;
    ipc_sem_post(sem_shm);
}

/* Glowna petla doboru grup na peron i rezerwacji miejsc. */
void platform_try_form_groups(int qid, cablecar_t *cablecar,
                              int sem_shm, int sem_chairs,
                              pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n)
{
    /* zapamiętuje, kogo wysłaliśmy w poprzednim trybie awaryjnym:
     * 0 – pieszy, 1 – rowerzysta.  Pozwala to naprzemiennie obsługiwać obu. */
    static int last_fallback_biker = 1;

    while (1) {
        /* 1. Dwóch rowerzystów razem */
        if (*bikers_n >= 2) {
            pid_t p1 = bikers[0];
            pid_t p2 = bikers[1];
            memmove(&bikers[0], &bikers[2], (*bikers_n - 2) * sizeof(pid_t));
            *bikers_n -= 2;
            pid_t pids[2] = { p1, p2 };
            reserve_seat(cablecar, sem_shm, sem_chairs, pids, 2, 4);
            platform_send_res(qid, p1);
            platform_send_res(qid, p2);
            continue;
        }

        /* 2. Jeden rowerzysta + piesi zajmujący w sumie 2 miejsca (1+1 lub 2) */
        if (*bikers_n >= 1) {
            int idxs[4], idxs_n = 0;
            if (pick_peds_sum(peds, *peds_n, 2, idxs, &idxs_n)) {
                pid_t biker_pid = bikers[0];
                memmove(&bikers[0], &bikers[1], (*bikers_n - 1) * sizeof(pid_t));
                *bikers_n -= 1;
                pid_t pids[4]; int pids_n = 0;
                pids[pids_n++] = biker_pid;
                for (int i = 0; i < idxs_n; i++)
                    pids[pids_n++] = peds[idxs[i]].pid;
                reserve_seat(cablecar, sem_shm, sem_chairs, pids, pids_n, 4);
                platform_send_res(qid, biker_pid);
                for (int i = 0; i < idxs_n; i++)
                    platform_send_res(qid, peds[idxs[i]].pid);
                for (int i = idxs_n - 1; i >= 0; i--)
                    remove_ped_at(peds, peds_n, idxs[i]);
                continue;
            }
        }

        /* 3. Sami piesi – dobierz grupy o sumie = 4 */
        {
            int idxs[4], idxs_n = 0;
            if (pick_peds_sum(peds, *peds_n, 4, idxs, &idxs_n)) {
                pid_t pids[4]; int pids_n = 0;
                for (int i = 0; i < idxs_n; i++)
                    pids[pids_n++] = peds[idxs[i]].pid;
                reserve_seat(cablecar, sem_shm, sem_chairs, pids, pids_n, 4);
                for (int i = 0; i < idxs_n; i++)
                    platform_send_res(qid, peds[idxs[i]].pid);
                for (int i = idxs_n - 1; i >= 0; i--)
                    remove_ped_at(peds, peds_n, idxs[i]);
                continue;
            }
        }

        /* 4. Obsłuż pieszych większych niż 1 osoba. */
        {
            int idx_big = -1;
            for (int i = 0; i < *peds_n; i++) {
                if (peds[i].size > 1) { idx_big = i; break; }
            }
            if (idx_big >= 0) {
                pid_t pid  = peds[idx_big].pid;
                int size   = peds[idx_big].size;
                reserve_seat(cablecar, sem_shm, sem_chairs, &pid, 1, size);
                platform_send_res(qid, pid);
                remove_ped_at(peds, peds_n, idx_big);
                continue;
            }
        }

        /* 5. Awaryjne obsłużenie pojedynczych:
         * Naprzemiennie rowerzysta/pieszy, aby uniknąć zagłodzenia. */
        if (*bikers_n > 0 && *peds_n > 0) {
            if (last_fallback_biker) {
                /* Tym razem pieszy */
                pid_t pid = peds[0].pid;
                reserve_seat(cablecar, sem_shm, sem_chairs, &pid, 1, 1);
                platform_send_res(qid, pid);
                remove_ped_at(peds, peds_n, 0);
                last_fallback_biker = 0;
                continue;
            } else {
                /* Tym razem rowerzysta */
                pid_t biker_pid = bikers[0];
                memmove(&bikers[0], &bikers[1], (*bikers_n - 1) * sizeof(pid_t));
                *bikers_n -= 1;
                reserve_seat(cablecar, sem_shm, sem_chairs, &biker_pid, 1, 2);
                platform_send_res(qid, biker_pid);
                last_fallback_biker = 1;
                continue;
            }
        }

        /* Jeśli zostały tylko rowerzyści albo tylko piesi‑single, obsługujemy ich wprost */
        if (*bikers_n > 0) {
            pid_t biker_pid = bikers[0];
            memmove(&bikers[0], &bikers[1], (*bikers_n - 1) * sizeof(pid_t));
            *bikers_n -= 1;
            reserve_seat(cablecar, sem_shm, sem_chairs, &biker_pid, 1, 2);
            platform_send_res(qid, biker_pid);
            last_fallback_biker = 1;
            continue;
        }
        if (*peds_n > 0) {
            pid_t pid = peds[0].pid;
            reserve_seat(cablecar, sem_shm, sem_chairs, &pid, 1, 1);
            platform_send_res(qid, pid);
            remove_ped_at(peds, peds_n, 0);
            last_fallback_biker = 0;
            continue;
        }

        /* Brak oczekujących – kończymy */
        break;
    }
}
