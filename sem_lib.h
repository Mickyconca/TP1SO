#ifndef SEM_LIB
#define SEM_LIB
#include <semaphore.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#define SEM_NAME "/sem"
#define NAME_MAX 255
#define HANDLE_ERROR(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)
#endif

typedef struct
{
    sem_t *access;
    char name[NAME_MAX - 4];
} t_sem;

t_sem createSem(char *name);
void closeSem(t_sem *sem);
