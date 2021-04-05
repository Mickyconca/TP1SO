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

#define WRITE 1
#define READ 0

#define PATH_SLAVE "./Slave"
#define PATH_RESULTS "./results.txt"
#define NSLAVES 5
#define BF_SIZE 4096
#define INIT_TASKS 1
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
static void asignTasks(t_slave *slave, int *taskIndex, char *fileToAssign);
static void writeResults(t_shm *shareMem, char *buffer, FILE *fResults, t_sem *sem);
static void initialize(int argc, char const *argv[], t_shm *shareMem, t_sem *sem);
static void finalize(t_slave slaves[], int dimSlaves, t_sem *sem, t_shm *shareMem, FILE *fResults);

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

    int initialTasks = INIT_TASKS;
    createSlaves(nSlaves, slaves, initialTasks, tasks, &taskIndex);

    int tasksDone = 0;
    int pendingTasks = totalTasks - tasksDone;
    int nfds = -1;

    FILE *fResults = fopen(PATH_RESULTS, "w");
    sleep(2);

    char buffer[BF_SIZE + 2] = {0};
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
                buffer[dimRead + 1] = '\0';
                int nTasks = 0;
                
                for (int j = 0; j < dimRead; j++)
                {
                    if (buffer[j] == '\t')
                    {
                        nTasks++;
                        tasksDone++;
                        buffer[j] = 0;
                        writeResults(&shareMem, buffer, fResults, &sem);
                    }
                }

                slaves[i].ntasks -= nTasks;

                pendingTasks = totalTasks - tasksDone;

                // Se asignan si es necesario las tareas pendientes.
                if (slaves[i].ntasks >= initialTasks && taskIndex < totalTasks)
                {
                    asignTasks(&slaves[i], &taskIndex, (char *)tasks[taskIndex]);
                }
            }
        }
    }
    finalize(slaves, nSlaves, &sem, &shareMem, fResults);
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
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0)
    {
        HANDLE_ERROR("Error in Setvbuf");
    }

    checkFiles(argc - 1, argv + 1);

    *shareMem = createShm(SHM_NAME, (argc - 1) * BF_SIZE);
    *sem = createSem(SEM_NAME);
    printf("%i", argc - 1);
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

            if (close(slaveToMaster[READ]) < 0)
            {
                HANDLE_ERROR("Error closing slave WRITE to master");
            }
            if (dup2(slaveToMaster[WRITE], WRITE) < 0)
            {
                HANDLE_ERROR("Error dupping pipe");
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
    
    for (int i = 0; i < dim; i++)
    {
        FILE *fptr = fopen(files[i], "r");
        if (fptr == NULL)
        {
            HANDLE_ERROR("Please check the selected files");
        }
        fclose(fptr);
    }
    return 1;
}

static void writeResults(t_shm *shareMem, char *buffer, FILE *fResults, t_sem *sem)
{
    int cantRead = strlen(buffer);
    writeShm(shareMem, buffer, cantRead, 3);
   
    if (fwrite(buffer, sizeof(char), cantRead, fResults) == 0)
        HANDLE_ERROR("Error in file write");
    if (sem_post(sem->access) == -1)
        HANDLE_ERROR("Error in sem_post");
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

static void asignTasks(t_slave *slave, int *taskIndex, char *fileToAssign)
{
    int dim = strlen(fileToAssign);
    int ans = write(slave->receiveInfoFD, fileToAssign, dim);
    if (ans == -1)
    {
        HANDLE_ERROR("Error in write");
    }
    slave->ntasks++;
    (*taskIndex)++;
}

static void finalize(t_slave slaves[], int dimSlaves, t_sem *sem, t_shm *shareMem, FILE *fResults)
{
    endSlavery(slaves, dimSlaves);
    eraseSem(sem);
    eraseShm(shareMem);
    fclose(fResults);
}
