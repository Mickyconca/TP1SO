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
        tasksRead[bytesRead] = 0;
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
    
    if (tasksSize == 0)
    {
        HANDLE_ERROR("Error, no tasks found to be processed");
    }

    t_shm shareMem = joinShm(SHM_NAME, tasksSize * MAX);
    t_sem sem = createSem(SEM_NAME);
    
    int i = 0;
    do
    {
        sem_wait(sem.access);
        char *toRead = readShm(&shareMem);
        int cantRead = strlen(toRead);
        if (cantRead > 0)
            i++;
        printf("%s\n", toRead);
    } while (i < tasksSize);
   
    closeShm(&shareMem);
    closeSem(&sem);
    return 0;
}