#pragma once
#include <sys/types.h>
#include <stdint.h>

#define IPC_ENV_QID "CABLECAR_QID"

// priorytety w kolejce
#define MT_VIP_OR_CTRL 1
#define MT_NORMAL      2

typedef enum {
    PASS_SINGLE = 1,
    PASS_TK1    = 2,
    PASS_TK2    = 3,
    PASS_TK3    = 4,
    PASS_DAY    = 5
} pass_type_t;

typedef enum {
    REQ_BUY      = 1,
    REQ_SHUTDOWN = 9
} req_opcode_t;

typedef struct {
    long mtype1;         // odbiera kasjer
    long mtype2;         // odsyla kasjer
    pid_t pid;           // PID turysty
    int tickets_nbr;      // liczba biletow 
    int discount_tickets_nbr;    // liczba biletow ze znizka
    uint8_t pass_type;   // pass_type_t kasjer ustawia
} ticket_req_res_t;


int  ipc_create_queue(void);
void ipc_set_env_qid(int qid);
int  ipc_get_qid_from_env(void);
int  ipc_destroy_queue(int qid);
// int  ipc_send_shutdown(int qid);
