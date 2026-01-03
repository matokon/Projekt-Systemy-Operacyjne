#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "simulation.h"
#include "ipc.h"
#include "tourist_utils.h"

int main(void) {
    srand(time(NULL) ^ getpid());

    int qid = ipc_get_qid_from_env();
    int platform_qid = tourist_get_env_int(IPC_ENV_PLATFORM_QID);
    int sem_gate4 = ipc_get_sem_from_env(IPC_ENV_SEM_GATE4);
    int sem_gate3 = ipc_get_sem_from_env(IPC_ENV_SEM_GATE3);
    int sem_inside = ipc_get_sem_from_env(IPC_ENV_SEM_INSIDE);

    int tickets_nbr = 1;
    int discount_tickets_nbr = 0;
    int age = tourist_handle_children(&tickets_nbr, &discount_tickets_nbr);
    int children_cnt = tickets_nbr - 1;

    int is_vip = rand_vip_1pct();
    int is_biker = (rand() % 2);

    ticket_msg_t req;
    tourist_fill_ticket_request(&req, age, is_vip, is_biker, tickets_nbr, discount_tickets_nbr);

    printf(CLR_GREEN"    TURYSTA %d: ide do kasy (qid=%d) VIP=%d age=%d biker=%d children=%d tickets=%d disc_tickets=%d\n" RESET,
           getpid(), qid, is_vip, age, is_biker, children_cnt, req.tickets_nbr, req.discount_tickets_nbr);

    if (ipc_send(qid, &req) < 0) return 1;

    ticket_msg_t res;
    memset(&res, 0, sizeof(res));
    if (ipc_recv(qid, (long)getpid(), &res, 0) < 0) return 1;

    if (res.status == ST_OK) {
        if (tourist_do_lower_gate(res.pass_id, sem_inside, sem_gate4) < 0) return 1;
        int plat = tourist_do_platform_stage(res.pass_id, is_biker, platform_qid, sem_inside, sem_gate3);
        if (plat != 0) return (plat < 0) ? 1 : 0;
        printf(CLR_GREEN"    TURYSTA %d: dostalem pass_id=%u pass_type=%d discount=%d%%\n" RESET,
               getpid(), res.pass_id, res.assigned_pass, res.discount_applied);
    } else {
        printf(CLR_RED_B"    TURYSTA %d: odmowa status=%d\n" RESET, getpid(), res.status);
    }

    return 0;
}
