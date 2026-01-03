#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ipc.h"

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static key_t make_key(void) {
    key_t k = ftok(".", 'L');
    if (k == (key_t)-1) {
        perror("ftok");
        exit(1);
    }
    return k;
}

static key_t make_key_with_id(char proj_id) {
    key_t k = ftok(".", proj_id);
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

int ipc_create_queue_with_id(char proj_id) {
    key_t k = make_key_with_id(proj_id);
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

int ipc_create_sem(char proj_id, int init_val) {
    key_t k = make_key_with_id(proj_id);
    int semid = semget(k, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    union semun arg;
    arg.val = init_val;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl(SETVAL)");
        exit(1);
    }
    return semid;
}

void ipc_set_env_sem(const char *env_name, int semid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", semid);
    if (setenv(env_name, buf, 1) != 0) {
        perror("setenv");
        exit(1);
    }
}

int ipc_get_sem_from_env(const char *env_name) {
    const char *s = getenv(env_name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", env_name);
        exit(1);
    }
    return atoi(s);
}

int ipc_destroy_sem(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl(IPC_RMID)");
        return -1;
    }
    return 0;
}

int ipc_sem_wait(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    for (;;) {
        if (semop(semid, &op, 1) == 0) return 0;
        if (errno == EINTR) continue;
        perror("semop(wait)");
        return -1;
    }
}

int ipc_sem_post(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    for (;;) {
        if (semop(semid, &op, 1) == 0) return 0;
        if (errno == EINTR) continue;
        perror("semop(post)");
        return -1;
    }
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

int ipc_send_platform(int qid, const platform_msg_t *m) {
    for (;;) {
        if (msgsnd(qid, m, PLATFORM_MSGSZ, 0) == 0) return 0;
        if (errno == EINTR) continue;
        perror("msgsnd(platform)");
        return -1;
    }
}

int ipc_recv_platform(int qid, long mtype, platform_msg_t *m, int flags) {
    for (;;) {
        ssize_t r = msgrcv(qid, m, PLATFORM_MSGSZ, mtype, flags);
        if (r >= 0) return (int)r;
        if (errno == EINTR) continue;
        perror("msgrcv(platform)");
        return -1;
    }
}
