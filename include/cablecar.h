#pragma once
#include <sys/types.h>

#define IPC_ENV_QID "CABLECAR_QID"                 /* kolejka biletowa */
#define IPC_ENV_PLATFORM_QID "CABLECAR_PLATFORM_QID" /* kolejka peronowa */
#define IPC_ENV_SEM_GATE4 "CABLECAR_SEM_GATE4"     /* bramka dolna (pozwolenie na wejście) */
#define IPC_ENV_SEM_GATE3 "CABLECAR_SEM_GATE3"     /* bramka platformy (wejście do wagonu) */
#define IPC_ENV_SEM_INSIDE "CABLECAR_SEM_INSIDE"   /* licznik osób w strefie dolnej/peronu */
#define IPC_ENV_SHM_CABLECAR "CABLECAR_SHM_CABLECAR" /* shm z kolejką krzesełek i PIDami pracowników */
#define IPC_ENV_SEM_SHM "CABLECAR_SEM_SHM"         /* mutex do dostępu do shm kolejki */
#define IPC_ENV_SEM_CHAIRS "CABLECAR_SEM_CHAIRS"   /* wolne miejsca w wagonach (krzesłach) */
#define IPC_ENV_SEM_TOURISTS "CABLECAR_SEM_TOURISTS" /* limit równoległych procesów turystów */
#define IPC_ENV_TP "CABLECAR_TP"                   /* czas startu generowania (Tp) */
#define IPC_ENV_TK "CABLECAR_TK"                   /* czas zamknięcia peronu (Tk) */
#define IPC_ENV_START "CABLECAR_START"             /* znacznik czasu startu symulacji (time_t) */
#define IPC_ENV_SEM_EXIT2 "CABLECAR_SEM_EXIT2"     /* dwie równoległe ścieżki wyjścia górnego */
#define IPC_ENV_STOP_ONCE "CABLECAR_STOP_ONCE"     /* flaga jednorazowej pauzy emp1/emp2 */

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
    seat_t seats[72];      /* Ring an pamieci dzielonej */
    int head;              /* indeks do zdejmowania foteli */
    int tail;              /* indeks do dodawania foteli */
    int occupied;          /* bieżąca liczba zajętych foteli */
    pid_t emp1_pid;        /* PID pracownika peronu */
    pid_t emp2_pid;        /* PID pracownika kolei */
} cablecar_t;

void cablecar_init(cablecar_t *cablecar);
