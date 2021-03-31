// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOLVER "minisat"
#define MAX 4096 // PIPE_BUF
static void runTask(char *task);

int main(int argc, char const *argv[])
{
    // setvbuf(stdout, NULL, _IONBF, 0);
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    {
        perror("Error in Setvbuf");
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i < argc; i++)
    {
        // printf("Argv %d: %s.\n", i, argv[i]);
        runTask((char *)argv[i]);
    }
    char tasks[MAX + 1] = {0};
    int dimRead;

    while ((dimRead = read(STDIN_FILENO, tasks, MAX)) != 0)
    {
        if (dimRead == -1)
        {
            perror("Erorr in read");
            exit(EXIT_FAILURE);
        }
        tasks[dimRead] = 0;
        runTask(tasks);
    }
    return 0;
}

static void runTask(char *task)
{
    char command[MAX + 1];
    char output[MAX + 1];
    sprintf(command, "%s %s | grep -o -e \"Number of.*[0-9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\"", SOLVER, task);
    FILE *outputStream;
    if ((outputStream = popen(command, "r")) == NULL)
    {
        perror("Erorr in popen");
        exit(EXIT_FAILURE);
    }

    int count = fread(output, sizeof(char), MAX, outputStream);

    if (ferror(outputStream))
    {
        perror("Error in fread");
        exit(EXIT_FAILURE);
    }

    output[count] = 0;

    printf("PID: %d\nFilename: %s\n%s\t", getpid(), task, output);

    if (pclose(outputStream) == -1)
    {
        perror("Error in pclose");
        exit(EXIT_FAILURE);
    }
}