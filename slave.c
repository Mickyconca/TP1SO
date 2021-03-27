#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    sleep(5);
    printf("Funciona %s.\n", argv[1]);
}