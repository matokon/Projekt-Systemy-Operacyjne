#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include "simulation.h"
#include "cablecar.h"
#include "ipc.h"
#include "main_utils.h"
#define duration_sec 5
#define INSIDE_LIMIT 10

int main() {

    printf(CLR_PINK"[MAIN] %d Start programu" RESET "\n", getpid());

    int qid = ipc_create_queue();
    ipc_set_env_qid(qid);
    int platform_qid = ipc_create_queue_with_id('P');
    set_env_int(IPC_ENV_PLATFORM_QID, platform_qid);
    int sem_gate4 = ipc_create_sem('G', 4);
    int sem_gate3 = ipc_create_sem('T', 3);
    int sem_inside = ipc_create_sem('N', INSIDE_LIMIT);
    int sem_shm = ipc_create_sem('M', 1);
    int sem_chairs = ipc_create_sem('C', 36);
    fprintf(stderr, "[MAIN] semids gate4=%d gate3=%d inside=%d shm=%d chairs=%d\n",
            sem_gate4, sem_gate3, sem_inside, sem_shm, sem_chairs);
    
    set_env_sems(sem_gate4, sem_gate3, sem_inside, sem_chairs, sem_shm);

    int shmid = ipc_create_shm(sizeof(cablecar_t));
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    cablecar_init(cablecar);
    set_env_int(IPC_ENV_SHM_CABLECAR, shmid);

    pid_t cashier_pid = start_process("./cashier",  "cashier",  "cashier fork");
    pid_t emp1_pid    = start_process("./employee1","employee1","employee1 fork");
    pid_t emp2_pid    = start_process("./employee2","employee2","employee2 fork");

    srand(time(NULL) ^ getpid());

    printf(CLR_PINK"[MAIN %d] Zaczynam generować turystów przez %d s\n"RESET,
           getpid(), duration_sec);
    
    int tourist_count = 0;
    pid_t *tourists = spawn_processes_for_seconds_collect("./tourist","tourist", duration_sec, &tourist_count);

    platform_msg_t pshut;
    memset(&pshut, 0, sizeof(pshut));
    pshut.mtype = 1;
    pshut.kind = PLAT_SHUTDOWN;
    pshut.pid = getpid();
    ipc_send_platform(platform_qid, &pshut);
    wait_for_pids(tourists, tourist_count);
    free(tourists);

    printf(CLR_PINK"[MAIN %d] Wygenerowałem %d turystów" RESET "\n", getpid(), tourist_count);

    ticket_msg_t shut;
    memset(&shut, 0, sizeof(shut));
    shut.mtype = MT_VIP_OR_CTRL;
    shut.kind = MSG_SHUTDOWN;
    shut.pid   = getpid();
    ipc_send(qid, &shut);

    ipc_send_platform(platform_qid, &pshut);

    ticket_msg_t shut_ack;
    memset(&shut_ack, 0, sizeof(shut_ack));
    ipc_recv(qid, (long)getpid(), &shut_ack, 0);

    platform_msg_t pshut_ack;
    memset(&pshut_ack, 0, sizeof(pshut_ack));
    ipc_recv_platform(platform_qid, (long)getpid(), &pshut_ack, 0);

    kill(emp2_pid, SIGTERM);

    int status;
    waitpid(cashier_pid, &status, 0);
    waitpid(emp1_pid, &status, 0);
    waitpid(emp2_pid, &status, 0);

    cleanup_ipc(qid, platform_qid, sem_gate4, sem_gate3, sem_inside,
                sem_chairs, sem_shm, cablecar, shmid);

    printf(CLR_PINK"[MAIN %d] Koniec programu" RESET "\n", getpid());
    return 0;
}
