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
    strcpy(toReturn.name,name);

    toReturn.fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (toReturn.fd == -1)
    {
        HANDLE_ERROR("Error in shm_open");
    }
    if (ftruncate(toReturn.fd, toReturn.size) == -1)
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

void readShm(t_shm *shareMem, char *buffer, char token)
{
    int *rIndex = {0};
    *rIndex = shareMem->rIndex;
    char *readFrom = (char *)shareMem->address + *rIndex;
    int tokenIndex = strspn(shareMem->address + *rIndex, &token);
    memcpy(buffer, readFrom, tokenIndex - 1);
    *rIndex += tokenIndex;
    buffer[*rIndex] = 0;
}

void writeShm(t_shm *shareMem, char *fromWrite, int size)
{
    int writeIndex = shareMem->wIndex;
    char *toWrite = (char *)shareMem->address;
    for (int i = 0; i < size; i++)
    {
        toWrite[writeIndex++] = fromWrite[i];
    }
    toWrite[writeIndex] = 0;
}

void finalizeShm(t_shm *shareMem)
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
