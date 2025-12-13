#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "simulation.h"

int main() {

    printf(CLR_PINK"[MAIN] %d Start programu" RESET "\n", getpid());

    start_process("./cashier", "cashier", "cashier fork");
    start_process("./employee1", "employee1", "employee1 fork");
    start_process("./employee2", "employee2", "employee2 fork");

    srand(time(NULL) ^ getpid());

    int duration_sec = 5;

    printf(CLR_PINK"[MAIN %d] Zaczynam generować turystów przez %d s\n"RESET,
           getpid(), duration_sec);
    
    int tourist_amount = spawn_processes_for_seconds("./tourist", "tourist",duration_sec);
    printf(CLR_PINK"[MAIN %d] Wygenerowałem %d turystów" RESET "\n", getpid(), tourist_amount);


    printf(CLR_PINK"[MAIN %d] Zakończyłem generowanie nowych turystów, czekam na dzieci" RESET "\n",
           getpid());

    int status;
    while (wait(&status) > 0) {
    }

    printf(CLR_PINK"[MAIN %d] Koniec programu" RESET "\n", getpid());
    return 0;
}
