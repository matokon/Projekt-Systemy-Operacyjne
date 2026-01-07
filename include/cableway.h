#pragma once
#include <sys/types.h>

#define IPC_ENV_QID "CABLECAR_QID"
#define IPC_ENV_PLATFORM_QID "CABLECAR_PLATFORM_QID"
#define IPC_ENV_SEM_GATE4 "CABLECAR_SEM_GATE4"
#define IPC_ENV_SEM_GATE3 "CABLECAR_SEM_GATE3"
#define IPC_ENV_SEM_INSIDE "CABLECAR_SEM_INSIDE"


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
    int occupied;
} cableway_t;

