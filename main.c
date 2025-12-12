#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "simulation.h"

int main() {

    printf("[MAIN] %d Start programu\n", getpid());

    start_process("./cashier", "cashier", "cashier fork");
    start_process("./employee1", "employee1", "employee1 fork");
    start_process("./employee2", "employee2", "employee2 fork");

    srand(time(NULL) ^ getpid());

    int duration_sec = 5;

    printf("[MAIN %d] Zaczynam generować turystów przez %d s\n",
           getpid(), duration_sec);
    
    int tourist_amount = spawn_processes_for_seconds("./tourist", "tourist",duration_sec);
    printf("[MAIN %d] Wygenerowałem %d turystów\n", getpid(), tourist_amount);


    printf("[MAIN %d] Zakończyłem generowanie nowych turystów, czekam na dzieci\n",
           getpid());

    int status;
    while (wait(&status) > 0) {
    }

    printf("[MAIN %d] Koniec programu\n", getpid());
    return 0;
}
