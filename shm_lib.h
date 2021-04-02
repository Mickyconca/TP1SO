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
#endif
typedef struct
{
    int fd;        // use to read and write from the memory
    int rIndex;    // where should I read next
    int wIndex;    // where should I write
    int size;      // the size of the memory assigned
    char const *address; // use to map from a new process
} t_shm;

t_shm createShm(char *name, int size); // shm_open -> ftruncate -> mmap