/*
    Implementation of a shell
    matricule: FE20A038
    email: fomegnemeudje@outlook.com
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

// This function split a line in many parts delimited by a space
/* cp a.txt b.txt will become cp0a.txt0b.txt0
                              |-|-----|-----|
                      arg n°  |0|  1  |  2  |
*/
void parse(char *line, char **args, char *stream)
{
    while (*line != '\0')
    {
        // We mark the end of an argument
        while (*line == ' ' || *line == '\t' || *line == '\n')
        {
            // We mark and go to the next character
            *line++ = '\0';
        }

        // This argument will begin at this point of the line
        // we mark the begin and go to the next argument
        *args++ = line;

        // We skip the other part of the argument
        while (*line != ' ' && *line != '\0' && *line != '\t' && *line != '\n')
        {
            line++;
        }
    }

    // We mark the end of the argument list
    *--args = '\0';
}

// This function will execute the command in a child process
void execute(char **args)
{
    pid_t pid;

    // We create a child process
    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "Error: Cannot create a child process\n");
        exit(1);
    }
    else
    {
        // The process child execute the command
        if (pid == 0)
        {
            if (execvp(*args, args) < 0)
            {
                fprintf(stderr, "Error: Failed to execute the command\n");
                exit(1);
            }
        }
        // The process parent wait the child
        else
        {
            // wait for the completion
            wait(NULL);
        }
    }
}

int main(void)
{
    char line[1024];
    char *args[64];

    while (1)
    {
        printf("> ");

        // We flush the output screen
        fflush(stdout);

        // We get the line
        fgets(line, sizeof(line), stdin);

        // We parse the input, to get arguments
        parse(line, args);

        if (strcmp(args[0], "exit") == 0)
            exit(0);
        
        // We execute
        execute(args);
    }
} 