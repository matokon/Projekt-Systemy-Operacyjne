#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "simulation.h"
#include "ipc.h"

int main() {

    printf(CLR_PINK"[MAIN] %d Start programu" RESET "\n", getpid());

    int qid = ipc_create_queue();
    ipc_set_env_qid(qid);

    pid_t cashier_pid = start_process("./cashier",  "cashier",  "cashier fork");
    pid_t emp1_pid    = start_process("./employee1","employee1","employee1 fork");
    pid_t emp2_pid    = start_process("./employee2","employee2","employee2 fork");

    srand(time(NULL) ^ getpid());

    int duration_sec = 5;

    printf(CLR_PINK"[MAIN %d] Zaczynam generować turystów przez %d s\n"RESET,
           getpid(), duration_sec);
    
    int tourist_count = 0;
    pid_t *tourists = spawn_processes_for_seconds_collect("./tourist","tourist", duration_sec, &tourist_count);
    wait_for_pids(tourists, tourist_count);
    free(tourists);

    printf(CLR_PINK"[MAIN %d] Wygenerowałem %d turystów" RESET "\n", getpid(), tourist_count);

    ticket_msg_t shut;
    memset(&shut, 0, sizeof(shut));
    shut.mtype = MT_VIP_OR_CTRL;
    shut.kind = MSG_SHUTDOWN;
    shut.pid   = 0;
    ipc_send(qid, &shut);



    int status;
    waitpid(cashier_pid, &status, 0);
    waitpid(emp1_pid, &status, 0);
    waitpid(emp2_pid, &status, 0);

    ipc_destroy_queue(qid);

    printf(CLR_PINK"[MAIN %d] Koniec programu" RESET "\n", getpid());
    return 0;
}
