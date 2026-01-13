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
#define INSIDE_LIMIT 10

static void wait_for_cablecar_empty(cablecar_t *cablecar, int sem_shm) {
    for (;;) {
        int occ = 0;
        ipc_sem_wait(sem_shm);
        occ = cablecar->occupied;
        ipc_sem_post(sem_shm);
        if (occ == 0) break;
        sleep(1);
    }
}

static int parse_arg_int(const char *s, const char *name) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s || *s == '\0' || (end && *end != '\0') || v < 0 || v > 86400) {
        fprintf(stderr, "Niepoprawny %s: %s\n", name, s ? s : "(null)");
        exit(1);
    }
    return (int)v;
}

int main(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "Uzycie: %s <Tp> <Tk>\n", argv[0]);
        return 1;
    }
    int tp = parse_arg_int(argv[1], "Tp");
    int tk = parse_arg_int(argv[2], "Tk");
    if (tk <= tp) {
        fprintf(stderr, "Niepoprawny zakres: Tp=%d Tk=%d\n", tp, tk);
        return 1;
    }
    int duration_sec = tk - tp;

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
    int sem_exit2 = ipc_create_sem('U', 2);
    fprintf(stderr, "[MAIN] semids gate4=%d gate3=%d inside=%d shm=%d chairs=%d exit2=%d\n",
            sem_gate4, sem_gate3, sem_inside, sem_shm, sem_chairs, sem_exit2);
    
    set_env_sems(sem_gate4, sem_gate3, sem_inside, sem_chairs, sem_shm, sem_exit2);

    int shmid = ipc_create_shm(sizeof(cablecar_t));
    cablecar_t *cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    cablecar_init(cablecar);
    set_env_int(IPC_ENV_SHM_CABLECAR, shmid);
    set_env_int(IPC_ENV_TP, tp);
    set_env_int(IPC_ENV_TK, tk);
    set_env_int(IPC_ENV_START, (int)time(NULL));

    pid_t cashier_pid = start_process("./cashier",  "cashier",  "cashier fork");
    pid_t emp1_pid    = start_process("./employee1","employee1","employee1 fork");
    pid_t emp2_pid    = start_process("./employee2","employee2","employee2 fork");

    srand(time(NULL) ^ getpid());

    printf(CLR_PINK"[MAIN %d] Zaczynam generowac turystow przez %d s (Tp=%d Tk=%d)\n"RESET,
           getpid(), duration_sec, tp, tk);
    
    int tourist_count = 0;
    pid_t *tourists = spawn_processes_for_seconds_collect("./tourist","tourist", duration_sec, &tourist_count);

    platform_msg_t pshut;
    memset(&pshut, 0, sizeof(pshut));
    pshut.mtype = MT_VIP_OR_CTRL;
    pshut.kind = PLAT_SHUTDOWN;
    pshut.pid = getpid();
    ipc_send_platform(platform_qid, &pshut);
    wait_for_pids(tourists, tourist_count);
    free(tourists);

    printf(CLR_PINK"[MAIN %d] Wygenerowałem %d turystów" RESET "\n", getpid(), tourist_count);

    wait_for_cablecar_empty(cablecar, sem_shm);
    sleep(3);

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
                sem_chairs, sem_shm, sem_exit2, cablecar, shmid);

    printf(CLR_PINK"[MAIN %d] Koniec programu" RESET "\n", getpid());
    return 0;
}
