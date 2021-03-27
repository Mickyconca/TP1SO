#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
// ​./solve files/* | ./vista

//fd[0] read
//fd[1] write
#define WRITE 1
#define READ 0

#define FORK_ERROR -1
#define EXEC_ERROR -2
#define PIPE_ERROR -3
#define NARGUMENTS_ERROR -4
#define DUPPING_ERROR -5
#define CLOSE_ERROR -6

#define PATH_SLAVE "./slave"
#define PATH_RESULTS "./results.txt"

#define NSLAVES 5

typedef struct
{
    pid_t pid;
    int sendInfoFD;    //devuelve resultados, slaveToMaster
    int receiveInfoFD; //manda tarea, masterToSlave
} t_slave;
int create_slaves(t_slave slaves[]);
int check_files(int dim, char const *files[]);

int main(int argc, char const *argv[])
{
    if (argc <= 1)
    {
        printf("Please select files.\n");
        exit(NARGUMENTS_ERROR);
    }
    check_files(argc - 1, argv + 1);
    t_slave slaves[NSLAVES];
    create_slaves(slaves);

    return 0;
}

int create_slaves(t_slave slaves[])
{
    for (int i = 0; i < NSLAVES; i++)
    {
        int masterToSlave[2];
        pipe(masterToSlave);
        if (pipe(masterToSlave) < 0)
        {
            perror("Pipe Error Master");
            exit(PIPE_ERROR);
        }
        slaves[i].receiveInfoFD = masterToSlave[WRITE];

        int slaveToMaster[2];
        pipe(slaveToMaster);
        if (pipe(slaveToMaster) < 0)
        {
            perror("Pipe Error Slave ");
            exit(PIPE_ERROR);
        }
        slaves[i].sendInfoFD = slaveToMaster[READ];
        int pid = fork();
        if (pid == 0)
        {
            if (close(masterToSlave[WRITE]) < 0)
            {
                perror("Error closing slave WRITE to master");
                exit(CLOSE_ERROR);
            }
            if (dup2(masterToSlave[READ], READ) < 0)
            {
                perror("Error dupping pipe");
                exit(DUPPING_ERROR);
            }
            // close(masterToSlave[READ]);

            if (close(slaveToMaster[READ]) < 0)
            {
                perror("Error closing slave WRITE to master");
                exit(CLOSE_ERROR);
            }
            if (dup2(slaveToMaster[WRITE], WRITE) < 0)
            {
                perror("Error dupping");
                exit(DUPPING_ERROR);
            }
            char *args[] = {PATH_SLAVE, NULL};
            if (execv(args[0], args) < 0)
            {
                perror("Execv Error");
                exit(EXEC_ERROR);
            }
        }
        else if (pid < 0)
        {
            perror("Fork Error");
            exit(FORK_ERROR);
        }
        // Cierro el pipe de READ de masterToSlave
        if (close(masterToSlave[READ]) < 0)
        {
            perror("Error closing master READ to slave");
            exit(CLOSE_ERROR);
        }
        if (close(slaveToMaster[WRITE]) < 0)
        {
            perror("Error closing master READ to slave");
            exit(CLOSE_ERROR);
        }
        slaves[i].pid = pid;
        waitpid(slaves[i].pid, NULL, 0);
    }
    return 0;
}

int check_files(int dim, char const *files[])
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
