#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "cablecar.h"
#include "ipc.h"

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/* Tworzy prywatna kolejke komunikatow; blad konczy proces. */
int ipc_create_queue(void) {
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (qid == -1) {
        perror("msgget error(1)");
        exit(1);
    }
    return qid;
}



/* Zapisuje id kolejki do zmiennej srodowiskowej IPC_ENV_QID. */
void ipc_set_env_qid(int qid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", qid);
    if (setenv(IPC_ENV_QID, buf, 1) != 0) {
        perror("setenv error(2)");
        exit(1);
    }
}

/* Pobiera id kolejki z IPC_ENV_QID lub konczy proces, gdy brak. */
int ipc_get_qid_from_env(void) {
    const char *s = getenv(IPC_ENV_QID);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_QID);
        exit(1);
    }
    return (int)strtol(s, NULL, 10);
}

/* Odczytuje msg_qbytes dla kolejki qid; zwraca 0 przy bledzie. */
size_t ipc_get_qbytes(int qid) {
    struct msqid_ds ds;
    if (msgctl(qid, IPC_STAT, &ds) == -1) {
        perror("msgctl(IPC_STAT) get_qbytes");
        return 0;
    }
    return (size_t)ds.msg_qbytes;
}

/* Usuwa kolejke komunikatow qid (IPC_RMID). */
int ipc_destroy_queue(int qid) {
    if (msgctl(qid, IPC_RMID, NULL) == -1) {
        perror("msgctl error(3)");
        return -1;
    }
    return 0;
}

/* Wysyla komunikat z kontrola zapchania kolejki (progiem threshold). */
int ipc_send_with_backpressure(int qid, const ticket_msg_t *m, double threshold) {
    if (threshold <= 0.0) threshold = 0.7;
    if (threshold >= 1.0) threshold = 0.99;

    struct msqid_ds buf;
    size_t msg_size_bytes = TICKET_MSGSZ + sizeof(long);
    if (msg_size_bytes == 0) msg_size_bytes = sizeof(long) + 1;

    for (;;) {
        if (msgctl(qid, IPC_STAT, &buf) == -1) {
            perror("msgctl(IPC_STAT) backpressure");
            break;
        }
        size_t max_msgs = buf.msg_qbytes / msg_size_bytes;
        if (max_msgs == 0) max_msgs = 1;
        if (buf.msg_qnum < (size_t)(threshold * max_msgs)) {
            break;
        }
        usleep(100000);
    }
    return ipc_send(qid, m);
}

/* Liczy startowa wartosc semafora straznika na podstawie pojemnosci kolejki. */
int ipc_calc_guard_init(int qid, size_t msgsz_payload) {
    struct msqid_ds ds;
    if (msgctl(qid, IPC_STAT, &ds) == -1) {
        perror("msgctl(IPC_STAT) guard");
        return 1;
    }
    size_t full_msg = msgsz_payload + sizeof(long);
    if (full_msg == 0) return 1;
    size_t capacity = ds.msg_qbytes / full_msg;
    if (capacity == 0) capacity = 1;
    if (capacity > 1) capacity -= 1;
    if (capacity > (size_t)INT_MAX) capacity = INT_MAX;
    return (int)capacity;
}

/* Tworzy semafor System V (1 element) i ustawia wartosc poczatkowa. */
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

/* Zapisuje id semafora w zmiennej env env_name. */
void ipc_set_env_sem(const char *env_name, int semid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", semid);
    if (setenv(env_name, buf, 1) != 0) {
        perror("setenv error(6)");
        exit(1);
    }
}

/* Pobiera id semafora z env env_name lub konczy proces, gdy brak. */
int ipc_get_sem_from_env(const char *env_name) {
    const char *s = getenv(env_name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", env_name);
        exit(1);
    }
    return (int)strtol(s, NULL, 10);
}

/* Usuwa semafor System V (IPC_RMID). */
int ipc_destroy_sem(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl error(7)");
        return -1;
    }
    return 0;
}

/* Sprawdza czy dla qid istnieje semafor straznika zapisany w env; zwraca id lub -1. */
static int guard_for_qid(int qid) {
    const char *s = getenv(IPC_ENV_QID);
    if (s && *s && (int)strtol(s, NULL, 10) == qid) {
        const char *g = getenv(IPC_ENV_SEM_Q_GUARD);
        if (g && *g) return (int)strtol(g, NULL, 10);
    }
    s = getenv(IPC_ENV_PLATFORM_QID);
    if (s && *s && (int)strtol(s, NULL, 10) == qid) {
        const char *g = getenv(IPC_ENV_SEM_PQ_GUARD);
        if (g && *g) return (int)strtol(g, NULL, 10);
    }
    return -1;
}

/* Operacja P na semaforze; ponawia przy EINTR. */
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

/* Operacja V na semaforze; ponawia przy EINTR. */
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

/* Wysyla ticket_msg_t na kolejke qid z uwzglednieniem semafora straznika. */
int ipc_send(int qid, const ticket_msg_t *m) {
    int guard = guard_for_qid(qid);
    if (guard > 0 && ipc_sem_wait(guard) < 0) return -1;
    for (;;) {
        if (msgsnd(qid, m, TICKET_MSGSZ, 0) == 0) return 0;
        if (errno == EINTR) continue;
        if (guard > 0) ipc_sem_post(guard);
        perror("msgsnd error(10)");
        return -1;
    }
}

/* Odbiera ticket_msg_t; po sukcesie zwalnia semafor straznika. */
int ipc_recv(int qid, long mtype, ticket_msg_t *m, int flags) {
    int guard = guard_for_qid(qid);
    for (;;) {
        ssize_t r = msgrcv(qid, m, TICKET_MSGSZ, mtype, flags);
        if (r >= 0) {
            if (guard > 0) ipc_sem_post(guard);
            return (int)r;
        }
        if (errno == EINTR) continue;
        perror("msgrcv error(11)");
        return -1;
    }
}

/* Wysyla platform_msg_t na kolejke platformowa z ewentualnym straznikiem. */
int ipc_send_platform(int qid, const platform_msg_t *m) {
    int guard = guard_for_qid(qid);
    if (guard > 0 && ipc_sem_wait(guard) < 0) return -1;
    for (;;) {
        if (msgsnd(qid, m, PLATFORM_MSGSZ, 0) == 0) return 0;
        if (errno == EINTR) continue;
        if (guard > 0) ipc_sem_post(guard);
        perror("msgsnd(platform) error(12)");
        return -1;
    }
}

/* Odbiera platform_msg_t; po sukcesie zwalnia semafor straznika. */
int ipc_recv_platform(int qid, long mtype, platform_msg_t *m, int flags) {
    int guard = guard_for_qid(qid);
    for (;;) {
        ssize_t r = msgrcv(qid, m, PLATFORM_MSGSZ, mtype, flags);
        if (r >= 0) {
            if (guard > 0) ipc_sem_post(guard);
            return (int)r;
        }
        if (errno == EINTR) continue;
        perror("msgrcv(platform) error(13)");
        return -1;
    }
}

/* Tworzy segment pamieci wspoldzielonej o zadanym rozmiarze. */
int ipc_create_shm(size_t size) {
    int shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget error(14)");
        exit(1);
    }
    return shmid;
}

/* Dolacza wskazany segment pamieci do procesu; blad konczy proces. */
void* ipc_attach_shm(int shmid) {
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        perror("shmat error(15)");
        exit(1);
    }
    return addr;
}

/* Odlacza segment pamieci; zwraca 0 lub -1. */
int ipc_detach_shm(void *addr) {
    if (shmdt(addr) == -1) {
        perror("shmdt error(16)");
        return -1;
    }
    return 0;
}

/* Usuwa segment pamieci (IPC_RMID); zwraca 0 lub -1. */
int ipc_destroy_shm(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl(IPC_RMID) error(16)");
        return -1;
    }
    return 0;
}
