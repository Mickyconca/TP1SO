// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>

#include "sem_lib.h"
#include "shm_lib.h"
// ​./solve files/* | ./vista

//fd[0] read
//fd[1] write
#define WRITE 1
#define READ 0

#define PATH_SLAVE "./slave"
#define PATH_RESULTS "./results.txt"
#define NSLAVES 5
#define MIN_TASKS 2
#define BF_SIZE 4096
#define HANDLE_ERROR(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)
#undef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef struct
{
    pid_t pid;
    int sendInfoFD;    // Devuelve resultados, slaveToMaster.
    int receiveInfoFD; // Manda tarea, masterToSlave.
    int ntasks;        // Tareas siendo ejecutadas por el esclavo.
    int flagEOF;
} t_slave;
static int createSlaves(int dimSlaves, t_slave slaves[], int initialTasks, char *files[], int *taskIndex);
static void endSlavery(t_slave slaves[], int dimSlaves);
static int checkFiles(int dim, char const *files[]);
static void writeResults(t_shm* shareMem, char *buffer, int size, FILE *fResults);
static void initialize(int argc, char const *argv[], t_shm *shareMem, t_sem *sem);
static void finalize(t_slave slaves[], int dimSlaves, t_sem *sem, t_shm *shareMem);

int main(int argc, char const *argv[])
{
    int totalTasks = argc - 1;
    t_shm shareMem;
    t_sem sem;
    initialize(argc, argv, &shareMem, &sem);

    int taskIndex = 0;
    int nSlaves = (totalTasks <= NSLAVES) ? totalTasks : NSLAVES;
    t_slave slaves[nSlaves];
    char **tasks = (char **)argv + 1;
    int initialTasks = (totalTasks >= nSlaves * MIN_TASKS) ? MIN_TASKS : MIN_TASKS - 1;
    createSlaves(nSlaves, slaves, initialTasks, tasks, &taskIndex);

    int tasksDone = 0;
    int pendingTasks = totalTasks - tasksDone;
    int nfds = -1;
    // Esto de open no va a quedar aca porque no tienee sentido.
    // Al tener la sharememory conviene guardar todo ahi y al final de
    // todo imprimirlo en el archivo. Para testear escribimos todo el tiempo.
    FILE *fResults = fopen(PATH_RESULTS, "w");
    
    sleep(2);
    // printf("%i\n", totalTasks); // le enviamos en el pipeo al view la cantidad de tareas a resolver.
    char buffer[BF_SIZE + 1] = {0};
    fd_set fdSlaves;
    while (pendingTasks > 0)
    {
        FD_ZERO(&fdSlaves);
        int dimRead = 0;
        // Cargamos los fd al set para que el select puede vigilar esos fd.
        for (int i = 0; i < nSlaves; i++)
        {
            if (!slaves[i].flagEOF)
            {
                int fdAux = slaves[i].sendInfoFD;
                FD_SET(fdAux, &fdSlaves);
                nfds = MAX(nfds, fdAux);
            }
        }
        // Usamos select para vigilar los fd read del pipe slave to master.
        int res;
        if ((res = select(nfds + 1, &fdSlaves, NULL, NULL, NULL)) == -1)
        {
            HANDLE_ERROR("Error in Select");
        }

        for (int i = 0; res > 0 && i < nSlaves; i++)
        {
            int fdCurrent = slaves[i].sendInfoFD;
            if (FD_ISSET(fdCurrent, &fdSlaves))
            {
                // entonces puedo leer lo que se encuentra en el pipe.
                // ver cuantas tareas resolvio y actualizar las tareas completadas
                dimRead = read(fdCurrent, buffer, BF_SIZE);

                if (dimRead == -1)
                {
                    HANDLE_ERROR("Error in Read");
                }
                if (dimRead == 0)
                {
                    slaves[i].flagEOF = 1;
                }
                buffer[dimRead] = '\0';
                int nTasks = 0;
                for (int j = 0, k = 0; j < dimRead; j++, k++)
                {
                    if (buffer[j] == '\t')
                    {
                        nTasks++;
                        tasksDone++;
                        // printf("Tasks Done: %i\n", tasksDone);
                        if (sem_post(sem.access) == -1)
                            HANDLE_ERROR("Error in sem_post");
                        writeResults(&shareMem, buffer, j+1, fResults);
                        k = 0;   
                    }
                }
                slaves[i].ntasks -= nTasks;
                printf("%s", buffer);
                pendingTasks = totalTasks - tasksDone;

                // Se asignan si es necesario las tareas pendientes.
                if (slaves[i].ntasks >= initialTasks && taskIndex < totalTasks)
                {
                    char *fileToAssign = (char *)tasks[taskIndex];
                    int dim = strlen(fileToAssign);
                    int ans = write(slaves[i].receiveInfoFD, fileToAssign, dim);
                    if (ans == -1)
                    {
                        HANDLE_ERROR("Error in write");
                    }
                    taskIndex++;
                }
            }
        }
    }
    finalize(slaves, nSlaves, &sem, &shareMem);
    return 0;
}

void initialize(int argc, char const *argv[], t_shm *shareMem, t_sem *sem)
{
    if (argc <= 1)
    {
        HANDLE_ERROR("Please select files.\n");
    }
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    {
        HANDLE_ERROR("Error in Setvbuf");
    }
    checkFiles(argc - 1, argv + 1);

    printf("%i", argc - 1);

    *shareMem = createShm(SHM_NAME, BF_SIZE);
    *sem = createSem(SEM_NAME);
}

static int createSlaves(int dimSlaves, t_slave slaves[], int initialTasks, char *files[], int *taskIndex)
{
    char *slaveArguments[initialTasks + 2]; // contando Nombre y NULL del final.
    slaveArguments[0] = PATH_SLAVE;
    slaveArguments[initialTasks + 1] = NULL;
    for (int i = 0; i < dimSlaves; i++)
    {
        int masterToSlave[2];
        pipe(masterToSlave);
        if (pipe(masterToSlave) < 0)
        {
            HANDLE_ERROR("Pipe Error Master");
        }
        slaves[i].receiveInfoFD = masterToSlave[WRITE];

        int slaveToMaster[2];
        pipe(slaveToMaster);
        if (pipe(slaveToMaster) < 0)
        {
            HANDLE_ERROR("Pipe Error Slave ");
        }
        slaves[i].sendInfoFD = slaveToMaster[READ];
        slaves[i].flagEOF = 0;
        int pid = fork();
        if (pid == 0)
        {
            if (close(masterToSlave[WRITE]) < 0)
            {
                HANDLE_ERROR("Error closing slave WRITE to master");
            }
            if (dup2(masterToSlave[READ], READ) < 0)
            {
                HANDLE_ERROR("Error dupping pipe");
            }
            // close(masterToSlave[READ]);

            if (close(slaveToMaster[READ]) < 0)
            {
                HANDLE_ERROR("Error closing slave WRITE to master");
            }
            if (dup2(slaveToMaster[WRITE], WRITE) < 0)
            {
                HANDLE_ERROR("Error dupping");
            }

            for (int j = 1; j < initialTasks + 1; j++)
            {
                slaveArguments[j] = (char *)files[(*taskIndex) + j - 1];
                slaves[i].ntasks++;
            }
            if (execv(slaveArguments[0], slaveArguments) < 0)
            {
                HANDLE_ERROR("Execv Error");
            }
        }
        else if (pid < 0)
        {
            HANDLE_ERROR("Fork Error");
        }
        // Cierro el pipe de READ de masterToSlave
        if (close(masterToSlave[READ]) < 0)
        {
            HANDLE_ERROR("Error closing master READ to slave");
        }
        if (close(slaveToMaster[WRITE]) < 0)
        {
            HANDLE_ERROR("Error closing slave WRITE to master");
        }
        slaves[i].pid = pid;
        (*taskIndex) += initialTasks;
    }
    return 0;
}

static int checkFiles(int dim, char const *files[])
{
    FILE *fptr;
    for (int i = 0; i < dim; i++)
    {
        fptr = fopen(files[i], "r");
        if (fptr == NULL)
        {
            HANDLE_ERROR("Please check the selected files");
        }
        fclose(fptr);
    }
    return 1;
}

static void writeResults(t_shm* shareMem, char *buffer, int size, FILE *fResults)
{
    writeShm(shareMem, buffer, size);
    // Escribo en la shareMemory
    buffer[size-1] = '\n'; // removemos \t
    // Escribo en el archivo.
    if (fwrite(buffer, sizeof(char), size, fResults) == 0)
        HANDLE_ERROR("Error in file write");
}
static void endSlavery(t_slave slaves[], int dimSlaves)
{
    for (int i = 0; i < dimSlaves; i++)
    {
        //Cierro los dos pipes del esclavo
        if (close(slaves[i].receiveInfoFD) == -1)
        {
            HANDLE_ERROR("Error closing slave pipes");
        }
        if (close(slaves[i].sendInfoFD) == -1)
        {
            HANDLE_ERROR("Error closing slave pipes");
        }
    }
    // Hago el wait de los esclavos para que no queden zombies
    for (int i = 0; i < dimSlaves; i++)
    {
        // Cierro los dos pipes del esclavo
        if (wait(NULL) == -1)
        {
            HANDLE_ERROR("Error in wait for slave");
        }
    }
}
static void finalize(t_slave slaves[], int dimSlaves, t_sem *sem, t_shm *shareMem)
{
    endSlavery(slaves, dimSlaves);
    closeSem(sem);
    eraseShm(shareMem);
}
