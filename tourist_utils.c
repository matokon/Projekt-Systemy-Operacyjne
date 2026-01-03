#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "tourist_utils.h"
#include "simulation.h"
#include "ipc.h"

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
    fprintf(f, "%u;%ld;gate=platform\n", pass_id, (long)now);
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

int tourist_do_lower_gate(uint32_t pass_id, int sem_inside, int sem_gate4) {
    if (ipc_sem_wait(sem_inside) < 0) return -1;
    if (ipc_sem_wait(sem_gate4) < 0) return -1;
    log_lower_gate(pass_id);
    ipc_sem_post(sem_gate4);
    int wait_ms = (rand() % 2000) + 500;
    usleep((useconds_t)wait_ms * 1000);
    return 0;
}

int tourist_do_platform_stage(uint32_t pass_id, int is_biker, int platform_qid,
                              int sem_inside, int sem_gate3) {
    platform_msg_t preq;
    memset(&preq, 0, sizeof(preq));
    preq.mtype = 1;
    preq.kind = PLAT_REQ;
    preq.pid = getpid();
    preq.is_biker = is_biker;
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
    ipc_sem_post(sem_inside);
    return 0;
}
