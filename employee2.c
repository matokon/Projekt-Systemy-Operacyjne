#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("    Pracownik1 Start: %d\n", getpid());
    sleep(14);
    printf("    Pracownik1 Koniec: %d\n", getpid());
    return 0;
}