#pragma once
#include <sys/types.h>
#include "cablecar.h"

typedef struct {
    pid_t pid;
    int size;
} ped_group_t;

/*
 * Wysyla przez kolejke platformowa odpowiedz PLAT_RES do danego pid.
 * qid - id kolejki platformowej, pid - proces oczekujacy na odpowiedz.
 */
void platform_send_res(int qid, pid_t pid);
/*
 * Wysyla do procesu pid komunikat PLAT_SHUTDOWN z kolejki platformowej qid.
 */
void platform_send_shutdown(int qid, pid_t pid);
/*
 * Oprania kolejke oczekujacych w stanie zamykania: wysyla PLAT_SHUTDOWN do
 * zaparkowanych rowerzystow i pieszych, po czym zeruje ich licznik.
 */
void platform_flush_shutdown_waiters(int qid, pid_t *bikers, int *bikers_n,
                                     ped_group_t *peds, int *peds_n);
/*
 * Dodaje nowe zgloszenie na peron: albo do kolejki rowerzystow, albo pieszych
 * (z rozmiarem grupy). Nie przekracza limitu max_queue.
 */
void platform_enqueue_request(int is_biker, pid_t pid, int group_size,
                              pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n, int max_queue);
/*
 * Probuje utworzyc grupy do wagonikow na podstawie kolejek rowerzystow i
 * pieszych; rezerwuje miejsca w pamieci wspoldzielonej kabelka i wysyla
 * odpowiadajace komunikaty PLAT_RES.
 */
void platform_try_form_groups(int qid, cablecar_t *cablecar, int sem_shm, int sem_chairs,
                              pid_t *bikers, int *bikers_n,
                              ped_group_t *peds, int *peds_n);
