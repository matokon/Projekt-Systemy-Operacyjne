#include "ipc.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static key_t make_key(void) {
    key_t k = ftok(".", 'L');
    if (k == (key_t)-1) {
        perror("ftok");
        exit(1);
    }
    return k;
}

int ipc_create_queue(void) {
    key_t k = make_key();
    int qid = msgget(k, IPC_CREAT | 0666);
    if (qid == -1) {
        perror("msgget");
        exit(1);
    }
    return qid;
}

void ipc_set_env_qid(int qid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", qid);
    if (setenv(IPC_ENV_QID, buf, 1) != 0) {
        perror("setenv");
        exit(1);
    }
}

int ipc_get_qid_from_env(void) {
    const char *s = getenv(IPC_ENV_QID);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_QID);
        exit(1);
    }
    return atoi(s);
}

int ipc_destroy_queue(int qid) {
    if (msgctl(qid, IPC_RMID, NULL) == -1) {
        perror("msgctl(IPC_RMID)");
        return -1;
    }
    return 0;
}

// int ipc_send_shutdown(int qid) { //todo 
//     cashier_req_t req;
//     memset(&req, 0, sizeof(req));
//     req.mtype  = MT_VIP_OR_CTRL;
//     req.opcode = REQ_SHUTDOWN;
//     req.pid    = 0;

//     if (msgsnd(qid, &req, sizeof(req) - sizeof(long), 0) == -1) {
//         perror("msgsnd shutdown");
//         return -1;
//     }
//     return 0;
// }
