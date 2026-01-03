#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "simulation.h"
#include "ipc.h"
#include "platform_queue.h"

int main() {
    printf(CLR_CYAN"    Pracownik1 Start: %d" RESET "\n", getpid());

    const char *s = getenv(IPC_ENV_PLATFORM_QID);
    if (!s || !*s) {
        fprintf(stderr, "Brak %s w env\n", IPC_ENV_PLATFORM_QID);
        return 1;
    }
    int platform_qid = atoi(s);

    enum { MAX_QUEUE = 1024 };
    pid_t bikers[MAX_QUEUE];
    pid_t peds[MAX_QUEUE];
    int bikers_n = 0;
    int peds_n = 0;
    int closing = 0;

    for (;;) {
        platform_msg_t req;
        memset(&req, 0, sizeof(req));
        if (ipc_recv_platform(platform_qid, 1, &req, 0) < 0) break;

        if (req.kind == PLAT_SHUTDOWN) {
            if (closing) {
                printf(CLR_CYAN"    Pracownik1 %d: shutdown final" RESET "\n", getpid());
                break;
            }
            closing = 1;
            printf(CLR_CYAN"    Pracownik1 %d: zamkniecie peronu" RESET "\n", getpid());
            platform_flush_shutdown_waiters(platform_qid, bikers, &bikers_n, peds, &peds_n);
            continue;
        }
        if (req.kind != PLAT_REQ || req.pid <= 0) {
            continue;
        }

        if (closing) {
            platform_send_shutdown(platform_qid, req.pid);
            continue;
        }

        platform_enqueue_request(req.is_biker, req.pid, bikers, &bikers_n, peds, &peds_n, MAX_QUEUE);
        platform_try_form_groups(platform_qid, bikers, &bikers_n, peds, &peds_n);
    }

    printf(CLR_CYAN"    Pracownik1 Koniec: %d" RESET "\n", getpid());
    return 0;
}
