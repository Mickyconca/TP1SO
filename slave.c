#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOLVER "minisat"
#define MAX 4096
static void runTask(char *task);
//  | grep -o -e "Number of.*[0-9]\+" -e "CPU time.*" -e ".*SATISFIABLE"

int main(int argc, char const *argv[])
{
    // if (argc - 1 == 1)
    //     sleep(5);
    for (int i = 0; i < argc; i++)
        runTask((char *)argv[i]);
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