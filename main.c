#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "simulation.h"

int main() {
    pid_t pid;

    printf("[MAIN] %d Start programu\n", getpid());

    start_process("./cashier", "cashier", "cashier fork");
    start_process("./employee1", "employee1", "employee1 fork");
    start_process("./employee2", "employee2", "employee2 fork");

    srand(time(NULL) ^ getpid());

    int czas_generacji = 5;
    time_t start = time(NULL);

    printf("[MAIN %d] Zaczynam generować turystów przez %d s\n",
           getpid(), czas_generacji);

    while (1) {
        int ms = (rand() % 900) + 100;
        usleep(ms * 1000);

        if (time(NULL) - start >= czas_generacji) {
            break;
        }

        pid = fork();
        if (pid == -1) {
            perror("fork tourist");
            exit(1);
        }
        if (pid == 0) {
            execl("./tourist", "tourist", (char *)NULL);
            perror("execl tourist");
            exit(1);
        }
    }


    printf("[MAIN %d] Zakończyłem generowanie nowych turystów, czekam na dzieci\n",
           getpid());

    int status;
    while (wait(&status) > 0) {
    }

    printf("[MAIN %d] Koniec programu\n", getpid());
    return 0;
}
