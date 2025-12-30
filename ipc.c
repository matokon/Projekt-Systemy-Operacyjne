#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ipc.h"

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
    snprintf(buf, sizeof(buf), "%d", qid); // toread
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

int ipc_send(int qid, const ticket_msg_t *m) {
    for (;;) {
        if (msgsnd(qid, m, TICKET_MSGSZ, 0) == 0) return 0;
        if (errno == EINTR) continue;
        perror("msgsnd");
        return -1;
    }
}

int ipc_recv(int qid, long mtype, ticket_msg_t *m, int flags) {
    for (;;) {
        ssize_t r = msgrcv(qid, m, TICKET_MSGSZ, mtype, flags);
        if (r >= 0) return (int)r;
        if (errno == EINTR) continue;
        perror("msgrcv");
        return -1;
    }
}
