#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/msg.h>
#include <sched.h>
#include "tourist_utils.h"
#include "simulation.h"
#include "ipc.h"
#include "cablecar.h"

/* Dopisuje wiersz do report.txt dla przejścia przez podaną bramkę. */
static void log_report(uint32_t pass_id, const char *gate) {
    FILE *f = fopen("report.txt", "a");
    if (!f) {
        perror("fopen report.txt");
        return;
    }
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char ts[32] = {0};
    if (lt) {
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", lt);
    } else {
        snprintf(ts, sizeof(ts), "%ld", (long)now);
    }
    fprintf(f, "%u;%s;gate=%s\n", pass_id, ts, gate);
    fclose(f);
}

/* Zwraca bieżący czas MONOTONIC w mikrosekundach. */
static long long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

/* Pobiera wartość zmiennej środowiskowej jako int (lub kończy proces). */
int tourist_get_env_int(const char *name) {
    const char *s = getenv(name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", name);
        exit(1);
    }
    return (int)strtol(s, NULL, 10);
}
/* Losuje dzieci (max 2), zwiększa bilety/znizki, uruchamia wątki dzieci; zwraca wiek rodzica. */
int tourist_handle_children(int *tickets_nbr, int *discount_tickets_nbr) {
    int children_cnt = 0;
    int age = rand_age();
    while (age < 8 && children_cnt < 2) {
        children_cnt++;
        (*tickets_nbr)++;
        (*discount_tickets_nbr)++;
        printf(CLR_RED_B"    TURYSTA %d: wylosowalem dziecko #%d (age=%d) -> tworze watek dziecka\n" RESET,
               getpid(), children_cnt, age);
        spawn_child_thread();
        age = rand_age();
    }
    while (age < 8) {
        age = rand_age();
    }
    return age;
}
/* Wypełnia strukturę żądania biletu danymi turysty. */
void tourist_fill_ticket_request(ticket_msg_t *req, int age, int is_vip, int is_biker,
                                 int tickets_nbr, int discount_tickets_nbr) {
    memset(req, 0, sizeof(*req));
    req->tickets_nbr = tickets_nbr;
    req->discount_tickets_nbr = discount_tickets_nbr;
    req->mtype = is_vip ? MT_VIP_OR_CTRL : MT_NORMAL;
    req->kind  = MSG_TICKET_REQ;
    req->pid   = getpid();
    req->age   = age;
    req->is_vip = is_vip;
    req->is_biker = is_biker;
    req->requested_pass = rand_pass_or_zero();
}
/* Obsługa dolnej bramki: weryfikuje ważność, rezerwuje inside/gate4, loguje wejście. */
int tourist_do_lower_gate(uint32_t pass_id, pass_type_t pass_type, int valid_until,
                          int sem_inside, int sem_gate4, int group_size) {
    if (sim_is_closed()) {
        printf(CLR_RED_B"    TURYSTA %d: bramki zamkniete (po Tk)\n" RESET, getpid());
        return 1;
    }
    if (pass_type != PASS_SINGLE && sim_now() > valid_until) {
        printf(CLR_RED_B"    TURYSTA %d: karnet niewazny (po czasie)\n" RESET, getpid());
        return 1;
    }
    if (group_size < 1) group_size = 1;
    for (int i = 0; i < group_size; i++) {
        if (ipc_sem_wait(sem_inside) < 0) return -1;
    }
    if (ipc_sem_wait(sem_gate4) < 0) return -1;
    log_report(pass_id, "lower");
    ipc_sem_post(sem_gate4);
    // int wait_ms = (rand() % 2000) + 500;
    // usleep((useconds_t)wait_ms * 1000);
    return group_size;
}
/* Wysyła PLAT_REQ i czeka (z limitem czasu) na PLAT_RES/PLAT_SHUTDOWN; oddaje sem_inside przy rezygnacji. */
int tourist_do_platform_stage(uint32_t pass_id, int is_biker, int is_vip, int group_size, int platform_qid,
                              int sem_inside, int sem_gate3) {
    platform_msg_t preq;
    memset(&preq, 0, sizeof(preq));
    preq.mtype = is_vip ? MT_VIP_OR_CTRL : MT_NORMAL;
    preq.kind = PLAT_REQ;
    preq.pid = getpid();
    preq.is_biker = is_biker;
    preq.group_size = group_size;
    preq.pass_id = pass_id;

    if (ipc_send_platform_nowait(platform_qid, &preq) < 0) {
        if (errno == EAGAIN) {
            printf(CLR_RED_B"    TURYSTA %d: kolejka peronu pelna, wychodze\n" RESET, getpid());
        }
        for (int i = 0; i < (group_size < 1 ? 1 : group_size); i++) ipc_sem_post(sem_inside);
        return 1;
    }

    platform_msg_t pres;
    memset(&pres, 0, sizeof(pres));

    /* Czekamy na odpowiedz z limitem czasu (~5s), aby nie wisiec bez konca. */
    const long long deadline = now_us() + 5000000;
    for (;;) {
        ssize_t r = msgrcv(platform_qid, &pres, PLATFORM_MSGSZ, (long)getpid(), IPC_NOWAIT);
        if (r >= 0) break;
        if (errno == EINTR) continue;
        if (errno == ENOMSG) {
            if (sim_is_closed() || now_us() >= deadline) {
                for (int i = 0; i < (group_size < 1 ? 1 : group_size); i++) ipc_sem_post(sem_inside);
                printf(CLR_RED_B"    TURYSTA %d: brak odpowiedzi peronu, rezygnuje\n" RESET, getpid());
                return 1;
            }
            sched_yield();
            continue;
        }  
        for (int i = 0; i < (group_size < 1 ? 1 : group_size); i++) ipc_sem_post(sem_inside);
        return 1;
    }

    if (pres.kind == PLAT_SHUTDOWN) {
        if (group_size < 1) group_size = 1;
        for (int i = 0; i < group_size; i++) ipc_sem_post(sem_inside);
        printf(CLR_RED_B"    TURYSTA %d: peron zamkniety, wychodze\n" RESET, getpid());
        return 1;
    }

    if (ipc_sem_wait(sem_gate3) < 0) return -1;
    log_report(pass_id, "platform");
    ipc_sem_post(sem_gate3);
    if (group_size < 1) group_size = 1;
    for (int i = 0; i < group_size; i++) {
        ipc_sem_post(sem_inside);
    }
    return 0;
}
/*Wybiera czas przejazdu*/ 
static int pick_trail_time(void) {
    int r = rand() % 3;
    if (r == 0) return TRAIL_T1;
    if (r == 1) return TRAIL_T2;
    return TRAIL_T3;
}

/* Przejście górne: pobiera sem_exit2, loguje przejazd, zwalnia sem_exit2. */
int tourist_do_upper_exit(int is_biker, int sem_exit2) {
    if (ipc_sem_wait(sem_exit2) < 0) return -1;
    // usleep(200 * 1000);
    log_report(0, "upper");
    ipc_sem_post(sem_exit2);
    if (is_biker) {
        // sleep(pick_trail_time());
    }
    return 0;
}
