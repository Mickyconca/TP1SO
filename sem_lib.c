// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "sem_lib.h"

t_sem createSem(char *name)
{
    t_sem toReturn = {0};
    strcpy(toReturn.name, name);
    toReturn.access = sem_open(toReturn.name, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 0);
    if (toReturn.access == SEM_FAILED)
    {
        HANDLE_ERROR("Error in sem_open");
    }
    return toReturn;
}

void closeSem(t_sem *sem)
{
    if (sem_close(sem->access) == -1)
    {
        HANDLE_ERROR("Error in sem_close");
    }
}

void eraseSem(t_sem *sem)
{
    if (sem_close(sem->access) == -1)
    {
        HANDLE_ERROR("Error in sem_close");
    }
    if (sem_unlink(sem->name) == -1)
    {
        HANDLE_ERROR("Error in sem_unlink");
    }
}