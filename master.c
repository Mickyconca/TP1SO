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
#define SHM_NAME "/shm"
#define SEM_NAME "/sem"
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
int createSlaves(int dimSlaves, t_slave slaves[], int initialTasks, char *files[], int *taskIndex);
int checkFiles(int dim, char const *files[]);

int main(int argc, char const *argv[])
{
    if (argc <= 1)
    {
        printf("Please select files.\n");
        exit(EXIT_FAILURE);
    }
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    {
        perror("Error in Setvbuf");
        exit(EXIT_FAILURE);
    }
    checkFiles(argc - 1, argv + 1);
    int totalTasks = argc - 1;

    printf("Total tasks: %i\n", totalTasks);
    char **tasks = (char **)argv + 1;
    int taskIndex = 0;
    int nSlaves = (totalTasks <= NSLAVES) ? totalTasks : NSLAVES;
    int initialTasks = (totalTasks >= nSlaves * MIN_TASKS) ? MIN_TASKS : MIN_TASKS - 1;
    t_slave slaves[nSlaves];
    createSlaves(nSlaves, slaves, initialTasks, tasks, &taskIndex);
    int tasksDone = 0;
    int pendingTasks = totalTasks - tasksDone;
    int nfds = -1;
    // Esto de open no va a quedar aca porque no tienee sentido.
    // Al tener la sharememory conviene guardar todo ahi y al final de
    // todo imprimirlo en el archivo. Para testear escribimos todo el tiempo.
    FILE *fResults = fopen(PATH_RESULTS, "w");
    sleep(2);

    printf("Tareas asignadas: %i\n", taskIndex);

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
            perror("Error in Select");
            exit(EXIT_FAILURE);
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
                    perror("Error in Read");
                    exit(EXIT_FAILURE);
                }
                if (dimRead == 0)
                {
                    slaves[i].flagEOF = 1;
                }
                buffer[dimRead] = '\0';
                int nTasks = 0;
                for (int j = 0; j < dimRead; j++)
                {
                    if (buffer[j] == '\t')
                    {
                        nTasks++;
                        tasksDone++;
                        printf("Tasks Done: %i\n", tasksDone);
                        buffer[j] = '\n'; // removemos \t
                        // Escribo en el archivo.
                        if (fwrite(buffer, sizeof(char), j, fResults) == 0)
                            HANDLE_ERROR("Error in file write");
                    }
                }
                slaves[i].ntasks -= nTasks;
                printf("%s", buffer);
                pendingTasks = totalTasks - tasksDone;

                // TODO: Asignar si es necesario las tareas pendientes.
                if (slaves[i].ntasks >= initialTasks && taskIndex < totalTasks)
                {
                    char *fileToAssign = (char *)tasks[taskIndex];
                    int dim = strlen(fileToAssign);
                    int ans = write(slaves[i].receiveInfoFD, fileToAssign, dim);
                    if (ans == -1)
                    {
                        perror("Error in write");
                        exit(EXIT_FAILURE);
                    }
                    taskIndex++;
                }
                // TODO: Luego escribo la respuesta para el vista, utilizando shareMemory.
            }
        }
    }
    // TODO: Un finalizar para cerrar todos los pipes y esas cosas.
    return 0;
}

int createSlaves(int dimSlaves, t_slave slaves[], int initialTasks, char *files[], int *taskIndex)
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
            perror("Pipe Error Master");
            exit(EXIT_FAILURE);
        }
        slaves[i].receiveInfoFD = masterToSlave[WRITE];

        int slaveToMaster[2];
        pipe(slaveToMaster);
        if (pipe(slaveToMaster) < 0)
        {
            perror("Pipe Error Slave ");
            exit(EXIT_FAILURE);
        }
        slaves[i].sendInfoFD = slaveToMaster[READ];
        slaves[i].flagEOF = 0;
        int pid = fork();
        if (pid == 0)
        {
            if (close(masterToSlave[WRITE]) < 0)
            {
                perror("Error closing slave WRITE to master");
                exit(EXIT_FAILURE);
            }
            if (dup2(masterToSlave[READ], READ) < 0)
            {
                perror("Error dupping pipe");
                exit(EXIT_FAILURE);
            }
            // close(masterToSlave[READ]);

            if (close(slaveToMaster[READ]) < 0)
            {
                perror("Error closing slave WRITE to master");
                exit(EXIT_FAILURE);
            }
            if (dup2(slaveToMaster[WRITE], WRITE) < 0)
            {
                perror("Error dupping");
                exit(EXIT_FAILURE);
            }

            for (int j = 1; j < initialTasks + 1; j++)
            {
                slaveArguments[j] = (char *)files[(*taskIndex) + j - 1];
                slaves[i].ntasks++;
            }
            if (execv(slaveArguments[0], slaveArguments) < 0)
            {
                perror("Execv Error");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0)
        {
            perror("Fork Error");
            exit(EXIT_FAILURE);
        }
        // Cierro el pipe de READ de masterToSlave
        if (close(masterToSlave[READ]) < 0)
        {
            perror("Error closing master READ to slave");
            exit(EXIT_FAILURE);
        }
        if (close(slaveToMaster[WRITE]) < 0)
        {
            perror("Error closing slave WRITE to master");
            exit(EXIT_FAILURE);
        }
        slaves[i].pid = pid;
        (*taskIndex) += initialTasks;
    }
    return 0;
}

int checkFiles(int dim, char const *files[])
{
    FILE *fptr;
    for (int i = 0; i < dim; i++)
    {
        fptr = fopen(files[i], "r");
        if (fptr == NULL)
        {
            perror("Please check the selected files");
            exit(-1);
        }
        //fclose(fptr);
    }
    return 1;
}
