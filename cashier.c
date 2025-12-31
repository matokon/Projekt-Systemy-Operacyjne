#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/msg.h>

#include "simulation.h"
#include "ipc.h"

typedef enum {
    HNDL_OK = 0,
    HNDL_CONTINUE,
    HNDL_BREAK
} handle_result_t;

static handle_result_t handle_prechecks(int qid, ticket_msg_t *msg)
{

    if (ipc_recv(qid, -MT_NORMAL, msg, 0) < 0) {
        return HNDL_BREAK;
    }

    if (msg->kind == MSG_SHUTDOWN) {
        printf(CLR_YELLOW"    Kasjer %d: dostałem SHUTDOWN, kończę" RESET "\n", getpid());
        return HNDL_BREAK;
    }

    if (msg->kind != MSG_TICKET_REQ || msg->pid <= 0 || msg->age < 0 || msg->age > 120) {
        ticket_msg_t res;
        memset(&res, 0, sizeof(res));
        res.mtype  = (long)msg->pid;
        res.kind   = MSG_TICKET_RES;
        res.pid    = msg->pid;
        res.status = ST_INVALID_REQUEST;
        ipc_send(qid, &res);
        return HNDL_CONTINUE;
    }

    return HNDL_OK;
}

int main() {
    srand(getpid());

    int qid = ipc_get_qid_from_env();
    printf(CLR_YELLOW"    Kasjer Start: %d (qid=%d)" RESET "\n", getpid(), qid);

    uint32_t next_pass_id = 1;

    for (;;) {
        ticket_msg_t msg;
        memset(&msg, 0, sizeof(msg));

        handle_result_t hr = handle_prechecks(qid, &msg);
        if (hr == HNDL_BREAK)    break;
        if (hr == HNDL_CONTINUE) continue;

        int discount = ((msg.age < 10 && msg.age > 7) || msg.age > 65) ? 25 : 0;
        
        ticket_msg_t res;
        memset(&res, 0, sizeof(res));
        res.mtype = (long)msg.pid;
        res.kind  = MSG_TICKET_RES;
        res.pid   = msg.pid;

        res.status = ST_OK;
        res.assigned_pass = pick_pass(msg.requested_pass);
        res.discount_applied = discount;
        res.pass_id = next_pass_id++;

        ipc_send(qid, &res);

        printf(CLR_YELLOW"    Kasjer %d: obsłużyłem pid=%d VIP=%d age=%d -> pass_id=%u discount=%d%%\n" RESET,
               getpid(), msg.pid, msg.is_vip, msg.age, res.pass_id, res.discount_applied);
    }

    printf(CLR_YELLOW"    Kasjer Koniec: %d" RESET "\n", getpid());
    return 0;
}
