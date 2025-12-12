#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("    Pracownik2 Start: %d\n", getpid());
    sleep(14);
    printf("    Pracownik2 Koniec: %d\n", getpid());
    return 0;
}