#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "tourist_utils.h"
#include "simulation.h"
#include "ipc.h"
#include "cablecar.h"

static void log_lower_gate(uint32_t pass_id) {
    FILE *f = fopen("lower_gate.log", "a");
    if (!f) {
        perror("fopen lower_gate.log");
        return;
    }
    time_t now = time(NULL);
    fprintf(f, "%u;%ld;gate=lower\n", pass_id, (long)now);
    fclose(f);
}

static void log_platform_gate(uint32_t pass_id) {
    FILE *f = fopen("platform_gate.log", "a");
    if (!f) {
        perror("fopen platform_gate.log");
        return;
    }
    time_t now = time(NULL);
    fprintf(f, "%u;%d;%ld;gate=platform\n", pass_id, getpid(), (long)now);
    fclose(f);
}

static void log_upper_gate(void) {
    FILE *f = fopen("upper_gate.log", "a");
    if (!f) {
        perror("fopen upper_gate.log");
        return;
    }
    time_t now = time(NULL);
    fprintf(f, "%ld;gate=upper\n", (long)now);
    fclose(f);
}

int tourist_get_env_int(const char *name) {
    const char *s = getenv(name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", name);
        exit(1);
    }
    return atoi(s);
}

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
    log_lower_gate(pass_id);
    ipc_sem_post(sem_gate4);
    int wait_ms = (rand() % 2000) + 500;
    usleep((useconds_t)wait_ms * 1000);
    return group_size;
}

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

    if (ipc_send_platform(platform_qid, &preq) < 0) return -1;

    platform_msg_t pres;
    memset(&pres, 0, sizeof(pres));
    if (ipc_recv_platform(platform_qid, (long)getpid(), &pres, 0) < 0) return -1;

    if (pres.kind == PLAT_SHUTDOWN) {
        ipc_sem_post(sem_inside);
        printf(CLR_RED_B"    TURYSTA %d: peron zamkniety, wychodze\n" RESET, getpid());
        return 1;
    }

    if (ipc_sem_wait(sem_gate3) < 0) return -1;
    log_platform_gate(pass_id);
    ipc_sem_post(sem_gate3);
    if (group_size < 1) group_size = 1;
    for (int i = 0; i < group_size; i++) {
        ipc_sem_post(sem_inside);
    }
    return 0;
}

static int pick_trail_time(void) {
    int r = rand() % 3;
    if (r == 0) return TRAIL_T1;
    if (r == 1) return TRAIL_T2;
    return TRAIL_T3;
}

int tourist_do_upper_exit(int is_biker, int sem_exit2) {
    if (ipc_sem_wait(sem_exit2) < 0) return -1;
    usleep(200 * 1000);
    log_upper_gate();
    ipc_sem_post(sem_exit2);
    if (is_biker) {
        sleep(pick_trail_time());
    }
    return 0;
}
