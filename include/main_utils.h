#pragma once
#include "cablecar.h"

/*
 * Zapisuje wartosc int w zmiennej srodowiskowej name (nadpisuje istniejaca).
 */
void set_env_int(const char *name, int value);
/*
 * Ustawia w env identyfikatory semaforow uzywanych w programie (gate4, gate3,
 * inside, chairs, shm, exit2).
 */
void set_env_sems(int sem_gate4, int sem_gate3, int sem_inside,
                  int sem_chairs, int sem_shm, int sem_exit2);
/*
 * Sprzata wszystkie obiekty IPC: kolejki, semafory, pamiec wspoldzielona.
 * Odlacza wskaznik cablecar i usuwa segment shmid.
 */
void cleanup_ipc(int qid, int platform_qid,
                 int sem_gate4, int sem_gate3, int sem_inside,
                 int sem_chairs, int sem_shm, int sem_exit2,
                 int sem_q_guard, int sem_pq_guard, int sem_emp_ready,
                 cablecar_t *cablecar, int shmid);
/*
 * Generuje raport na podstawie log_path i dopisuje podsumowanie do out_path.
 * Zwraca 0 gdy sukces, -1 przy bledzie.
 */
int generate_report(const char *log_path, const char *out_path);
/*
 * Czeka az wagonik bedzie pusty (occupied==0), synchronizujac sie sem_shm.
 */
void wait_for_cablecar_empty(cablecar_t *cablecar, int sem_shm);
/*
 * Zamienia ciag znakow na int (strtol) bez dodatkowej walidacji.
 */
int parse_arg_int(const char *s);
