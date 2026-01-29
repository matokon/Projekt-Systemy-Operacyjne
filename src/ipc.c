#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "cablecar.h"
#include "ipc.h"

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int ipc_create_queue(void) {
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (qid == -1) {
        perror("msgget error(1)");
        exit(1);
    }
    return qid;
}



void ipc_set_env_qid(int qid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", qid);
    if (setenv(IPC_ENV_QID, buf, 1) != 0) {
        perror("setenv error(2)");
        exit(1);
    }
}

int ipc_get_qid_from_env(void) {
    const char *s = getenv(IPC_ENV_QID);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_QID);
        exit(1);
    }
    return (int)strtol(s, NULL, 10);
}

int ipc_destroy_queue(int qid) {
    if (msgctl(qid, IPC_RMID, NULL) == -1) {
        perror("msgctl error(3)");
        return -1;
    }
    return 0;
}

int ipc_create_sem(char proj_id, int init_val) {
    (void)proj_id;
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget error(4)");
        exit(1);
    }
    union semun arg;
    arg.val = init_val;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl error(5)");
        exit(1);
    }
    return semid;
}

void ipc_set_env_sem(const char *env_name, int semid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", semid);
    if (setenv(env_name, buf, 1) != 0) {
        perror("setenv error(6)");
        exit(1);
    }
}

int ipc_get_sem_from_env(const char *env_name) {
    const char *s = getenv(env_name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", env_name);
        exit(1);
    }
    return (int)strtol(s, NULL, 10);
}

int ipc_destroy_sem(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl error(7)");
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
        perror("semop(wait) error(8)");
        return -1;
    }
}

int ipc_sem_wait_interruptible(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == 0) return 0;
    if (errno == EINTR) return -2;
    perror("semop(wait) error");
    return -1;
}

int ipc_sem_post(int semid) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    for (;;) {
        if (semop(semid, &op, 1) == 0) return 0;
        if (errno == EINTR) continue;
        perror("semop(post) error(9)");
        return -1;
    }
}

int ipc_send(int qid, const ticket_msg_t *m) {
    for (;;) {
        if (msgsnd(qid, m, TICKET_MSGSZ, IPC_NOWAIT) == 0) return 0;
        if (errno == EINTR) continue;
        if (errno == EAGAIN) return -2;
        perror("msgsnd error(10)");
        return -1;
    }
}

int ipc_recv(int qid, long mtype, ticket_msg_t *m, int flags) {
    for (;;) {
        ssize_t r = msgrcv(qid, m, TICKET_MSGSZ, mtype, flags);
        if (r >= 0) return (int)r;
        if (errno == EINTR) continue;
        perror("msgrcv error(11)");
        return -1;
    }
}

int ipc_send_platform(int qid, const platform_msg_t *m) {
    for (;;) {
        if (msgsnd(qid, m, PLATFORM_MSGSZ, IPC_NOWAIT) == 0) return 0;
        if (errno == EINTR) continue;
        if (errno == EAGAIN) return -2;
        perror("msgsnd(platform) error(12)");
        return -1;
    }
}

int ipc_recv_platform(int qid, long mtype, platform_msg_t *m, int flags) {
    for (;;) {
        ssize_t r = msgrcv(qid, m, PLATFORM_MSGSZ, mtype, flags);
        if (r >= 0) return (int)r;
        if (errno == EINTR) continue;
        if (errno == ENOMSG) return -2;
        perror("msgrcv(platform) error(13)");
        return -1;
    }
}

int ipc_create_shm(size_t size) {
    int shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget error(14)");
        exit(1);
    }
    return shmid;
}

void* ipc_attach_shm(int shmid) {
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        perror("shmat error(15)");
        exit(1);
    }
    return addr;
}

int ipc_detach_shm(void *addr) {
    if (shmdt(addr) == -1) {
        perror("shmdt error(16)");
        return -1;
    }
    return 0;
}

int ipc_destroy_shm(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl(IPC_RMID) error(16)");
        return -1;
    }
    return 0;
}
