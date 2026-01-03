#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "simulation.h"
#include "ipc.h"
#define duration_sec 5
#define INSIDE_LIMIT 10

static void set_env_int(const char *name, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    if (setenv(name, buf, 1) != 0) {
        perror("setenv");
        exit(1);
    }
}

int main() {

    printf(CLR_PINK"[MAIN] %d Start programu" RESET "\n", getpid());

    int qid = ipc_create_queue();
    ipc_set_env_qid(qid);
    int platform_qid = ipc_create_queue_with_id('P');
    set_env_int(IPC_ENV_PLATFORM_QID, platform_qid);
    int sem_gate4 = ipc_create_sem('G', 4);
    int sem_gate3 = ipc_create_sem('T', 3);
    int sem_inside = ipc_create_sem('N', INSIDE_LIMIT);
    ipc_set_env_sem(IPC_ENV_SEM_GATE4, sem_gate4);
    ipc_set_env_sem(IPC_ENV_SEM_GATE3, sem_gate3);
    ipc_set_env_sem(IPC_ENV_SEM_INSIDE, sem_inside);

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
    pshut.pid = 0;
    ipc_send_platform(platform_qid, &pshut);
    wait_for_pids(tourists, tourist_count);
    free(tourists);

    printf(CLR_PINK"[MAIN %d] Wygenerowałem %d turystów" RESET "\n", getpid(), tourist_count);

    ticket_msg_t shut;
    memset(&shut, 0, sizeof(shut));
    shut.mtype = MT_VIP_OR_CTRL;
    shut.kind = MSG_SHUTDOWN;
    shut.pid   = 0;
    ipc_send(qid, &shut);

    ipc_send_platform(platform_qid, &pshut);


    int status;
    waitpid(cashier_pid, &status, 0);
    waitpid(emp1_pid, &status, 0);
    waitpid(emp2_pid, &status, 0);

    ipc_destroy_queue(qid);
    ipc_destroy_queue(platform_qid);
    ipc_destroy_sem(sem_gate4);
    ipc_destroy_sem(sem_gate3);
    ipc_destroy_sem(sem_inside);

    printf(CLR_PINK"[MAIN %d] Koniec programu" RESET "\n", getpid());
    return 0;
}
