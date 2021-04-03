// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include "shm_lib.h"
#include "sem_lib.h"
#define SHM_NAME "/shm"
#define SEM_NAME "/sem"
#define MAX 4096 // PIPE_BUF

int main(int argc, char const *argv[])
{
    // t_shm shareMem;
    // t_sem sem;

    // shareMem = shm_open(SHM_NAME, O_RDONLY , 0444);
    // sem = sem_open(SEM_NAME, );
    return 0;
}