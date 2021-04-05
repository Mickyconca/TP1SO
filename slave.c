// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HANDLE_ERROR(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

#define SOLVER "minisat"
#define MAX 4096 // PIPE_BUF
static void runTask(char *task);

int main(int argc, char const *argv[])
{
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    {
        HANDLE_ERROR("Error in Setvbuf");
    }
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0)
    {
        HANDLE_ERROR("Error in Setvbuf");
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
            HANDLE_ERROR("Erorr in read");
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
        HANDLE_ERROR("Erorr in popen");
    }

    int count = fread(output, sizeof(char), MAX, outputStream);

    if (ferror(outputStream))
    {
        HANDLE_ERROR("Error in fread");
    }

    output[count] = 0;

    printf("PID: %d\nFilename: %s\n%s\t", getpid(), task, output);

    if (pclose(outputStream) == -1)
    {
        HANDLE_ERROR("Error in pclose");
    }
}