#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("    Kasjer Start: %d\n", getpid());
    sleep(10);
    printf("    Kasjer Koniec: %d\n", getpid());
    return 0;
}