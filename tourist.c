#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "simulation.h"
#include "ipc.h"

int main(void) {
    srand(time(NULL) ^ getpid());

    int qid = ipc_get_qid_from_env();

    int children_cnt = 0;
    int age = rand_age();

    ticket_msg_t req;
    memset(&req, 0, sizeof(req));

    req.tickets_nbr = 1;
    req.discount_tickets_nbr = 0;

    while (age < 8 && children_cnt < 2) {
        children_cnt++;
        req.tickets_nbr++;
        req.discount_tickets_nbr++;

        printf(CLR_RED_B"    TURYSTA %d: wylosowałem dziecko #%d (age=%d) -> tworzę wątek dziecka\n" RESET,
               getpid(), children_cnt, age);

        spawn_child_thread();

        age = rand_age();
    }

    while (age < 8) {
        age = rand_age();
    }

    int is_vip = rand_vip_1pct();
    int is_biker = (rand() % 2);

    req.mtype = is_vip ? MT_VIP_OR_CTRL : MT_NORMAL;
    req.kind  = MSG_TICKET_REQ;
    req.pid   = getpid();
    req.age   = age;
    req.is_vip = is_vip;
    req.is_biker = is_biker;
    req.requested_pass = rand_pass_or_zero();

    printf(CLR_GREEN"    TURYSTA %d: idę do kasy (qid=%d) VIP=%d age=%d biker=%d children=%d tickets=%d disc_tickets=%d\n" RESET,
           getpid(), qid, is_vip, age, is_biker, children_cnt, req.tickets_nbr, req.discount_tickets_nbr);


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
