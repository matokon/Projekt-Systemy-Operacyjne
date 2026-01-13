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
#define IPC_ENV_TP "CABLECAR_TP"
#define IPC_ENV_TK "CABLECAR_TK"
#define IPC_ENV_START "CABLECAR_START"
#define IPC_ENV_SEM_EXIT2 "CABLECAR_SEM_EXIT2"

#define TRAIL_T1 3
#define TRAIL_T2 5
#define TRAIL_T3 7



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
} cablecar_t;

void cablecar_init(cablecar_t *cablecar);
