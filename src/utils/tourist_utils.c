#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "tourist_utils.h"
#include "simulation.h"
#include "ipc.h"
#include "cablecar.h"

static cablecar_t *g_cablecar = NULL;
static int g_sem_shm = -1;
static int g_vip_marked = 0;
static int g_vip_handlers_installed = 0;

static void vip_clear(void);

/* Dopina pamiec cablecar i semafor shm na podstawie zmiennych srodowiskowych. */
static void ensure_cablecar_attached(void) {
    if (g_cablecar && g_sem_shm >= 0) return;
    const char *s = getenv(IPC_ENV_SHM_CABLECAR);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_SHM_CABLECAR);
        exit(1);
    }
    int shmid = (int)strtol(s, NULL, 10);
    g_cablecar = (cablecar_t*)ipc_attach_shm(shmid);
    g_sem_shm = ipc_get_sem_from_env(IPC_ENV_SEM_SHM);
}

/* Jednorazowo instaluje obsluge atexit i sygnalow do sprzatania licznika VIP. */
static void install_vip_guards(void) {
    if (g_vip_handlers_installed) return;
    g_vip_handlers_installed = 1;
    atexit(vip_clear);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (void(*)(int))vip_clear;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}

/* Ustawia flage VIP oczekujacego i zapewnia cleanup przy wyjsciu/sygnalach. */
static void vip_mark(void) {
    install_vip_guards();
    g_vip_marked = 1;
}

/* Zdejmuje flage VIP: dekrementuje licznik w cablecar (jesli dolaczony). */
static void vip_clear(void) {
    if (!g_vip_marked) return;
    if (!g_cablecar || g_sem_shm < 0) { g_vip_marked = 0; return; }
    ipc_sem_wait(g_sem_shm);
    if (g_cablecar->vip_waiting > 0) g_cablecar->vip_waiting--;
    ipc_sem_post(g_sem_shm);
    g_vip_marked = 0;
}

/* Dopisuje wpis do report.txt w formacie pass_id;timestamp;gate=... */
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

/* Pobiera zmienna env name jako int; przy braku konczy proces. */
int tourist_get_env_int(const char *name) {
    const char *s = getenv(name);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", name);
        exit(1);
    }
    return (int)strtol(s, NULL, 10);
}

/* Losuje dzieci (do 2) i ich bilety; zwraca wiek doroslego. */
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

/* Wypelnia zadanie zakupu biletu danymi turysty i losowanym karnetem. */
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

/* Etap dolnych bramek: obsluguje VIP, pobiera tokeny, loguje przejscie. */
int tourist_do_lower_gate(uint32_t pass_id, pass_type_t pass_type, int valid_until,
                          int sem_inside, int sem_gate4, int group_size, int is_vip) {
    ensure_cablecar_attached();

    if (sim_is_closed()) {
        printf(CLR_RED_B"    TURYSTA %d: bramki zamkniete (po Tk)\n" RESET, getpid());
        return 0;
    }
    if (pass_type != PASS_SINGLE && sim_now() > valid_until) {
        printf(CLR_RED_B"    TURYSTA %d: karnet niewazny (po czasie)\n" RESET, getpid());
        return 0;
    }
    if (group_size < 1) group_size = 1;

    if (is_vip) {
        ipc_sem_wait(g_sem_shm);
        g_cablecar->vip_waiting++;
        ipc_sem_post(g_sem_shm);
        vip_mark();
    } else {
        for (;;) {
            ipc_sem_wait(g_sem_shm);
            int vw = g_cablecar->vip_waiting;
            ipc_sem_post(g_sem_shm);
            if (vw == 0) break;
            /* Sprawdź zamknięcie podczas czekania na VIP */
            if (sim_is_closed()) {
                printf(CLR_RED_B"    TURYSTA %d: bramki zamkniete (czekajac na VIP)\n" RESET, getpid());
                
                return 0;
            }
            // usleep(1000);
        }
    }

    int taken_inside = 0;
    for (int i = 0; i < group_size; i++) {
        if (ipc_sem_wait(sem_inside) < 0) {
            // oddaj pobrane tokeny
            for (int j = 0; j < taken_inside; j++) {
                ipc_sem_post(sem_inside);
            }
            // jeśli to VIP, zmniejsz licznik oczekujących VIP
            if (is_vip) {
                vip_clear();
            }
            return -1;
        }
        // zwiększ licznik pobranych tokenów dla kolejnych iteracji
        taken_inside++;
    }

    /* VIP nie blokuje już zwykłych po wejściu do strefy N */
    if (is_vip) {
        vip_clear();
    }

    /* Sprawdź ponownie, czy w międzyczasie nie zamknięto stacji */
    if (sim_is_closed()) {
        for (int j = 0; j < taken_inside; j++) {
            ipc_sem_post(sem_inside);
        }
        if (is_vip) {
            /* zmniejsz licznik VIP waiting, jeśli trzeba */
            vip_clear();
        }
        printf(CLR_RED_B"    TURYSTA %d: bramki zamkniete (po wejsciu)\n" RESET, getpid());
        return 0;
    }

    /* Teraz możesz czekać na wolne stanowisko bramki; w razie błędu oddaj tokeny */
    if (ipc_sem_wait(sem_gate4) < 0) {
        for (int j = 0; j < taken_inside; j++) ipc_sem_post(sem_inside);
        if (is_vip) {
            vip_clear();
        }
        return -1;
    }



        log_report(pass_id, "lower");
        ipc_sem_post(sem_gate4);

        if (is_vip) {
            vip_clear();
        }

        int wait_ms = (rand() % 2000) + 500;
        // usleep((useconds_t)wait_ms * 1000);
        return group_size;
}

/* Etap peronu: wysyla PLAT_REQ, czeka na odpowiedz, loguje i zwalnia tokeny. */
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
    int release_n = group_size < 1 ? 1 : group_size;

    if (ipc_send_platform(platform_qid, &preq) < 0) {
        for (int i = 0; i < release_n; i++) {
            ipc_sem_post(sem_inside);
        }
        return -1;
    }
    platform_msg_t pres;
    memset(&pres, 0, sizeof(pres));
    if (ipc_recv_platform(platform_qid, (long)getpid(), &pres, 0) < 0) {
        for (int i = 0; i < release_n; i++) ipc_sem_post(sem_inside);
        return -1;
    }
    if (pres.kind == PLAT_SHUTDOWN) {
        for (int i = 0; i < release_n; i++) {
            ipc_sem_post(sem_inside);
        }
        printf(CLR_RED_B "    TURYSTA %d: peron zamkniety, wychodze\n" RESET, getpid());
        return 1;
    }

    if (ipc_sem_wait(sem_gate3) < 0) {
        for (int i = 0; i < release_n; i++) ipc_sem_post(sem_inside);
        return -1;
    }
    log_report(pass_id, "platform");
    ipc_sem_post(sem_gate3);
    if (group_size < 1) group_size = 1;
    for (int i = 0; i < group_size; i++) {
        ipc_sem_post(sem_inside);
    }
    return 0;
}

/* Losuje czas zjazdu rowerzysty (trasy T1..T3). */
static int pick_trail_time(void) {
    int r = rand() % 3;
    if (r == 0) return TRAIL_T1;
    if (r == 1) return TRAIL_T2;
    return TRAIL_T3;
}

/* Etap gornego wyjscia: blokuje sem_exit2, loguje, opcjonalnie czeka na trase. */
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
