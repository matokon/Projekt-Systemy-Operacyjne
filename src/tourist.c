#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"
#include "tourist_utils.h"

/* Glowny proces turysty: losuje parametry, kupuje bilet, przechodzi etapy przejazdu. */
int main(void) {
    srand(time(NULL) ^ getpid());
    // if ((rand() % 100) < 20) {
    //     printf(CLR_RED_B"    TURYSTA %d: rezygnuje z kolejki\n" RESET, getpid());
    //     return 0;
    // }

    int qid = ipc_get_qid_from_env();
    int platform_qid = tourist_get_env_int(IPC_ENV_PLATFORM_QID);
    int sem_gate4 = ipc_get_sem_from_env(IPC_ENV_SEM_GATE4);
    int sem_gate3 = ipc_get_sem_from_env(IPC_ENV_SEM_GATE3);
    int sem_inside = ipc_get_sem_from_env(IPC_ENV_SEM_INSIDE);
    int sem_exit2 = ipc_get_sem_from_env(IPC_ENV_SEM_EXIT2);

    int tickets_nbr = 1;
    int discount_tickets_nbr = 0;
    int age = tourist_handle_children(&tickets_nbr, &discount_tickets_nbr);
    int children_cnt = tickets_nbr - 1;

    int is_vip = rand_vip_1pct();
    int is_biker = (rand() % 2);
    if (children_cnt > 0) {
        is_biker = 0;
    }
    int group_size = tickets_nbr;

    ticket_msg_t req;
    tourist_fill_ticket_request(&req, age, is_vip, is_biker, tickets_nbr, discount_tickets_nbr);

    printf(CLR_GREEN"    TURYSTA %d: ide do kasy (qid=%d) VIP=%d age=%d biker=%d children=%d tickets=%d disc_tickets=%d\n" RESET,
           getpid(), qid, is_vip, age, is_biker, children_cnt, req.tickets_nbr, req.discount_tickets_nbr);

    if (ipc_send_with_backpressure(qid, &req, 0.7) < 0) return 1;

    ticket_msg_t res;
    memset(&res, 0, sizeof(res));
    if (ipc_recv(qid, (long)getpid(), &res, 0) < 0) return 1;

    if (res.status == ST_OK) {
        int ride_count = 0;
        for (;;) {
            int gate_tokens = tourist_do_lower_gate(res.pass_id, res.assigned_pass, res.valid_until,
                                                    sem_inside, sem_gate4, group_size, is_vip);
            if (gate_tokens <= 0) break;
            int plat = tourist_do_platform_stage(res.pass_id, is_biker, is_vip, group_size,
                                                 platform_qid, sem_inside, sem_gate3);
            if (plat != 0) break;
            if (tourist_do_upper_exit(is_biker, sem_exit2) < 0) return 1;
            ride_count++;
            printf(CLR_GREEN"    TURYSTA %d: pass_id=%u zjazd #%d pass_type=%d discount=%d%%\n" RESET,
                   getpid(), res.pass_id, ride_count, res.assigned_pass, res.discount_applied);

            if (res.assigned_pass == PASS_SINGLE) break;
            if (sim_now() > res.valid_until) break;
            // usleep(200 * 1000);
        }
    } else {
        printf(CLR_RED_B"    TURYSTA %d: odmowa status=%d\n" RESET, getpid(), res.status);
    }

    return 0;
}
