#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#define IPC_ENV_QID "CABLECAR_QID"
#define IPC_ENV_PLATFORM_QID "CABLECAR_PLATFORM_QID"
#define IPC_ENV_SEM_GATE4 "CABLECAR_SEM_GATE4"
#define IPC_ENV_SEM_GATE3 "CABLECAR_SEM_GATE3"
#define IPC_ENV_SEM_INSIDE "CABLECAR_SEM_INSIDE"

// priorytety w kolejce
#define MT_VIP_OR_CTRL 1
#define MT_NORMAL      2

typedef enum {
    MSG_TICKET_REQ = 1,
    MSG_TICKET_RES = 2,
    MSG_SHUTDOWN   = 3
} msg_kind_t;

typedef enum {
    PASS_SINGLE = 1,
    PASS_TK1    = 2,
    PASS_TK2    = 3,
    PASS_TK3    = 4,
    PASS_DAY    = 5
} pass_type_t;

typedef enum {
    ST_OK = 0,
    ST_REJECTED_CLOSED,     // po Tk / kasa zamknięta
    ST_INVALID_REQUEST      // np. błędne dane
} status_t;

typedef struct {
    long mtype;            // REQUEST: MT_VIP_OR_CTRL lub MT_NORMAL (priorytet w kolejce)
                           // RESPONSE: pid klienta (adresowanie odpowiedzi)

    msg_kind_t kind;       // czy to prośba czy odpowiedź
    pid_t pid;             // PID klienta

    //dane klienta / request
    int age;               // wiek
    int is_vip;            // 1 = VIP, 0 = nie
    int is_biker;          // 1 = rowerzysta, 0 = pieszy
    pass_type_t requested_pass;   // co chce kupić 0 = nie podano / losowo

    int tickets_nbr;            // ile biletów (jeśli kupuje dla grupy)
    int discount_tickets_nbr;   // ile ze zniżką (opcjonalnie)

    //odpowiedź kasjera
    status_t status;        // ST_OK albo powód odrzucenia
    pass_type_t assigned_pass; // co kasjer przydzielił
    int discount_applied;   // np. 25 albo 0 (nie dotyczy dzieci <8 wtedy patrzymy z discount_tickets_nbr)
    uint32_t pass_id;       // ID karnetu do rejestracji na bramkach

} ticket_msg_t;

#define TICKET_MSGSZ (sizeof(ticket_msg_t) - sizeof(long))

typedef enum {
    PLAT_REQ = 1,
    PLAT_RES = 2,
    PLAT_SHUTDOWN = 3
} platform_kind_t;

typedef struct {
    long mtype;            // REQUEST: 1, RESPONSE: pid klienta
    platform_kind_t kind;
    pid_t pid;
    int is_biker;
    uint32_t pass_id;
} platform_msg_t;

#define PLATFORM_MSGSZ (sizeof(platform_msg_t) - sizeof(long))

//ipc
int  ipc_create_queue(void);
int  ipc_create_queue_with_id(char proj_id);
void ipc_set_env_qid(int qid);
int  ipc_get_qid_from_env(void);
int  ipc_destroy_queue(int qid);
int ipc_send(int qid, const ticket_msg_t *m);
int ipc_recv(int qid, long mtype, ticket_msg_t *m, int flags);

int ipc_send_platform(int qid, const platform_msg_t *m);
int ipc_recv_platform(int qid, long mtype, platform_msg_t *m, int flags);

//semaphores
int  ipc_create_sem(char proj_id, int init_val);
void ipc_set_env_sem(const char *env_name, int semid);
int  ipc_get_sem_from_env(const char *env_name);
int  ipc_destroy_sem(int semid);
int  ipc_sem_wait(int semid);
int  ipc_sem_post(int semid);


// data_randomization
pass_type_t pick_pass(pass_type_t req);
pass_type_t rand_pass_or_zero(void);
int rand_vip_1pct(void);
int rand_age(void);
