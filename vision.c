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
#define MAX 4096   // PIPE_BUF
#define TOKEN '\t' // Token elegido para separar las tareas

int main(int argc, char const *argv[])
{
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    {
        HANDLE_ERROR("Error in Setvbuf");
    }
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0)
    {
        HANDLE_ERROR("Error in Setvbuf");
    }
    int tasksSize = 0;
    if (argc - 1 == 0) // Pipe
    {
        char tasksRead[MAX + 1];
        int bytesRead = 0;
        if ((bytesRead = read(STDIN_FILENO, tasksRead, 20)) == -1)
            HANDLE_ERROR("Error in read from pipe in vision");
        printf("bytesRead: %d\n", bytesRead);
        tasksRead[bytesRead] = 0;
        printf("%s.\n", tasksRead);
        tasksSize = strtol(tasksRead, NULL, 10);
    }
    else if (argc - 1 == 1) // Parametro
    {
        tasksSize = strtol(argv[1], NULL, 10);
    }
    else
    {
        HANDLE_ERROR("Error, wrong parameters in vision");
    }
    printf("TasksSize: %d\n", tasksSize);
    if (tasksSize == 0)
    {
        HANDLE_ERROR("Error, no tasks found to be processed");
    }

    t_shm shareMem = joinShm(SHM_NAME, MAX);
    t_sem sem = createSem(SEM_NAME);

    int keepReading = 0;
    char buffer[MAX];
    while (keepReading < tasksSize)
    {
        sem_wait(sem.access);
        readShm(&shareMem, buffer, TOKEN);
        keepReading++;
        printf("%s\n", buffer);
    }
    eraseShm(&shareMem);
    closeSem(&sem);
    return 0;
}