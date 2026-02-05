#pragma once
#include <sys/types.h>

#define IPC_ENV_QID "CABLECAR_QID"
#define IPC_ENV_PLATFORM_QID "CABLECAR_PLATFORM_QID"
#define IPC_ENV_SEM_GATE4 "CABLECAR_SEM_GATE4"
#define IPC_ENV_SEM_GATE3 "CABLECAR_SEM_GATE3"
#define IPC_ENV_SEM_INSIDE "CABLECAR_SEM_INSIDE"
#define IPC_ENV_SHM_CABLECAR "CABLECAR_SHM_CABLECAR"
#define IPC_ENV_SEM_SHM "CABLECAR_SEM_SHM"
#define IPC_ENV_SEM_CHAIRS "CABLECAR_SEM_CHAIRS"
#define IPC_ENV_SEM_Q_GUARD "CABLECAR_SEM_Q_GUARD"
#define IPC_ENV_SEM_PQ_GUARD "CABLECAR_SEM_PQ_GUARD"
#define IPC_ENV_SEM_EMP_READY "CABLECAR_SEM_EMP_READY"
#define IPC_ENV_TP "CABLECAR_TP"
#define IPC_ENV_TK "CABLECAR_TK"
#define IPC_ENV_START "CABLECAR_START"
#define IPC_ENV_SEM_EXIT2 "CABLECAR_SEM_EXIT2"
#define IPC_ENV_STOP_ONCE "CABLECAR_STOP_ONCE"

#define TRAIL_T1 3
#define TRAIL_T2 5
#define TRAIL_T3 7

#define TK1_DURATION 5
#define TK2_DURATION 10
#define TK3_DURATION 15



typedef enum {
    SEAT_FREE = 0,
    SEAT_OCCUPIED = 1
} seat_state_t;

typedef struct {
    seat_state_t state;
    int group_size;
    pid_t pids[4];
} seat_t;

typedef struct {
    seat_t seats[72];
    int head;
    int tail;
    int occupied;
    pid_t emp1_pid;
    pid_t emp2_pid;
    int vip_waiting;      // liczba VIP oczekujących na wejście (priorytet na dolnej stacji)
} cablecar_t;

/*
 * Inicjalizuje strukture cablecar: ustawia wskazniki kolejki na start, zeruje
 * zajetosc, pids pracownikow oraz licznik oczekujacych VIP.
 */
void cablecar_init(cablecar_t *cablecar);
