#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"
#include "sim_time.h"
#include "platform_queue.h"

int main() {
    printf(CLR_CYAN"    Pracownik1 Start: %d" RESET "\n", getpid());

    const char *s1 = getenv(IPC_ENV_PLATFORM_QID);
    if (!s1 || !*s1) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_PLATFORM_QID);
        return 1;
    }
    int platform_qid = atoi(s1);

    const char *s2 = getenv(IPC_ENV_SHM_CABLECAR);
    if (!s2 || !*s2) { 
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_SHM_CABLECAR);
        return 1;
     }
    int shmid = atoi(s2);
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    int sem_shm = ipc_get_sem_from_env(IPC_ENV_SEM_SHM);
    int sem_chairs = ipc_get_sem_from_env(IPC_ENV_SEM_CHAIRS);
    (void)cablecar;
    (void)sem_shm;
    (void)sem_chairs;


    enum { MAX_QUEUE = 1024 };
    pid_t bikers[MAX_QUEUE];
    ped_group_t peds[MAX_QUEUE];
    int bikers_n = 0;
    int peds_n = 0;
    int closing = 0;
    int gate_closed = 0;

    for (;;) {
        platform_msg_t req;
        memset(&req, 0, sizeof(req));
        if (ipc_recv_platform(platform_qid, 1, &req, 0) < 0) break;

        if (req.kind == PLAT_SHUTDOWN) {
            if (closing) {
                if (req.pid > 0) {
                    platform_msg_t ack;
                    memset(&ack, 0, sizeof(ack));
                    ack.mtype = (long)req.pid;
                    ack.kind = PLAT_SHUTDOWN_ACK;
                    ack.pid = getpid();
                    ipc_send_platform(platform_qid, &ack);
                }
                printf(CLR_CYAN"    Pracownik1 %d: shutdown final" RESET "\n", getpid());
                break;
            }
            closing = 1;
            printf(CLR_CYAN"    Pracownik1 %d: zamkniecie peronu" RESET "\n", getpid());
            platform_flush_shutdown_waiters(platform_qid, bikers, &bikers_n, peds, &peds_n);
            continue;
        }
        if (req.kind != PLAT_REQ || req.pid <= 0) {
            continue;
        }

        if (closing) {
            platform_send_shutdown(platform_qid, req.pid);
            continue;
        }

        if (sim_is_closed()) {
            if (!gate_closed) {
                printf(CLR_CYAN"    Pracownik1 %d: bramki peronu zamkniete (po Tk)\n" RESET, getpid());
                gate_closed = 1;
            }
            platform_send_shutdown(platform_qid, req.pid);
            continue;
        }

        platform_enqueue_request(req.is_biker, req.pid, req.group_size,
                                 bikers, &bikers_n, peds, &peds_n, MAX_QUEUE);
        platform_try_form_groups(platform_qid, cablecar, sem_shm, sem_chairs,
                                 bikers, &bikers_n, peds, &peds_n);
    }

    printf(CLR_CYAN"    Pracownik1 Koniec: %d" RESET "\n", getpid());
    return 0;
}
