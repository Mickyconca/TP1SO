// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "shm_lib.h"

// 1ero crear la memoria.
// 2do agregar algo a esa memoria.
// 2do bis leer de esa memoria.
// 3ero desbloquear la memoria.
// 4to eliminar la memoria.

t_shm createShm(char *name, int size)
{
    t_shm toReturn;
    toReturn.rIndex = 0;
    toReturn.wIndex = 0;
    toReturn.size = size;
    strcpy(toReturn.name, name);
    toReturn.fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (toReturn.fd == -1)
    {
        HANDLE_ERROR("Error in shm_open");
    }
    int truncateValue = ftruncate(toReturn.fd, toReturn.size);
    if (truncateValue == -1)
    {
        HANDLE_ERROR("Error in ftruncate");
    }
    int protection = PROT_WRITE | PROT_READ;
    int visibility = MAP_SHARED;
    toReturn.address = mmap(NULL, toReturn.size, protection, visibility, toReturn.fd, 0);
    if (toReturn.address == MAP_FAILED)
    {
        HANDLE_ERROR("Error in mmap");
    }
    return toReturn;
}

t_shm joinShm(char *name, int size)
{
    t_shm toReturn;
    strcpy(toReturn.name, name);
    if ((toReturn.fd = shm_open(name, O_RDONLY, 0)) == -1)
        HANDLE_ERROR("Error in shm_open in vision");
    if ((toReturn.address = mmap(NULL, size, PROT_READ, MAP_SHARED, toReturn.fd, 0)) == MAP_FAILED)
        HANDLE_ERROR("Error in mmap in vision");
    toReturn.wIndex = 0;
    toReturn.rIndex = 0;
    toReturn.size = size;

    return toReturn;
}


char *readShm(t_shm *shareMem)
{
    char *toReturn = shareMem->address + shareMem->rIndex;
    int size = strlen(toReturn);
    shareMem->rIndex += (size + 3); // OFFSET
    return toReturn;
}
void writeShm(t_shm *shareMem, char *fromWrite, int size, int offSet)
{
    memcpy(shareMem->address + shareMem->wIndex, fromWrite, size + 1);
    shareMem->wIndex += size + offSet;
}

void closeShm(t_shm *shareMem)
{
    int fd = shareMem->fd;
    int size = shareMem->size;
    char *address = shareMem->address;
    if (munmap(address, size) == -1)
    {
        HANDLE_ERROR("Error in munmap");
    }
    if (close(fd) == -1)
    {
        HANDLE_ERROR("Error in closing shm");
    }
}

void eraseShm(t_shm *shareMem)
{
    int fd = shareMem->fd;
    int size = shareMem->size;
    char *address = shareMem->address;
    char *name = shareMem->name;
    if (munmap(address, size) == -1)
    {
        HANDLE_ERROR("Error in munmap");
    }
    if (shm_unlink(name) == -1)
    {
        HANDLE_ERROR("Error in unlink");
    }
    if (close(fd) == -1)
    {
        HANDLE_ERROR("Error in closing shm");
    }
}