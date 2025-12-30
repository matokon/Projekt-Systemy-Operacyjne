#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "simulation.h"
#include "ipc.h"

static int rand_age(void) {
    return (rand() % 76) + 5;
}

static int rand_vip_1pct(void) {
    return (rand() % 100) == 0;
}

static pass_type_t rand_pass_or_zero(void) {
    if ((rand() % 4) == 0) return 0;
    return (pass_type_t)((rand() % 5) + 1);
}

int main(void) {
    srand(time(NULL) ^ getpid());

    int qid = ipc_get_qid_from_env();

    int age = rand_age();
    int is_vip = rand_vip_1pct();
    int is_biker = (rand() % 2);

    ticket_msg_t req;
    memset(&req, 0, sizeof(req));
    req.mtype = is_vip ? MT_VIP_OR_CTRL : MT_NORMAL;
    req.kind  = MSG_TICKET_REQ;
    req.pid   = getpid();
    req.age   = age;
    req.is_vip = is_vip;
    req.is_biker = is_biker;
    req.requested_pass = rand_pass_or_zero();
    req.tickets_nbr = 1;

    printf(CLR_GREEN"    TURYSTA %d: idę do kasy (qid=%d) VIP=%d age=%d biker=%d\n" RESET,
           getpid(), qid, is_vip, age, is_biker);

    if (ipc_send(qid, &req) < 0) return 1;

    ticket_msg_t res;
    memset(&res, 0, sizeof(res));
    if (ipc_recv(qid, (long)getpid(), &res, 0) < 0) return 1;

    if (res.status == ST_OK) {
        printf(CLR_GREEN"    TURYSTA %d: dostałem pass_id=%u pass_type=%d discount=%d%%\n" RESET,
               getpid(), res.pass_id, res.assigned_pass, res.discount_applied);
    } else {
        printf(CLR_RED_B"    TURYSTA %d: odmowa status=%d\n" RESET, getpid(), res.status);
    }

    return 0;
}
