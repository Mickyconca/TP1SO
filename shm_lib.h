#ifndef SHM_LIB
#define SHM_LIB
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#define SHM_NAME "/shm"
#define HANDLE_ERROR(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)
#endif

typedef struct
{
    char name[FILENAME_MAX];
    int fd;        // used to read and write from the memory
    int rIndex;    // where should I read next
    int wIndex;    // where should I write
    int size;      // the size of the memory assigned
    char *address; // use to map from a new process
} t_shm;

t_shm createShm(char *name, int size); // shm_open -> ftruncate -> mmap
t_shm joinShm(char *name, int size); 
void readShm(t_shm *shareMem, char *buffer, char token);
void writeShm(t_shm *shareMem, char *fromWrite, int size);
void closeShm(t_shm *shareMem);
void readShm(t_shm *shareMem, char *buffer, char token);
void eraseShm(t_shm *shareMem);