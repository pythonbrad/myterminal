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
#include <malloc.h>

#define MAX_NB_ARG 8
#define MAX_NB_CMD 8
#define MAX_ARG_LENGTH 8

// This function split a line in many parts delimited by a space
/* ls -la | grep *.exe will become ls0-la0|0grep0*.exe0
                                  |--|----||----|-----|
                            index |0 |  1 ||  0 |  1  |
                                  |-------||----------|
                        args n°   |   0   ||     1    |
*/
void parse(char *line, char *cmds[MAX_NB_CMD][MAX_NB_ARG])
{
    int cmd_id = 0;
    int arg_id = 0;

    while (*line != '\0')
    {
        // We mark the end of an argument
        while (*line == ' ' || *line == '\t' || *line == '\n' || *line == '|')
        {
            // We search an other command
            if (*line == '|')
            {
                cmds[cmd_id++][arg_id] = '\0';
                arg_id=0;
            }
            // We mark and go to the next character
            *line++ = '\0';
        }

        // This argument will begin at this point of the line
        // we mark the begin and go to the next argument
        cmds[cmd_id][arg_id++] = line;

        // We skip the other part of the argument
        while (*line != ' ' && *line != '\0' && *line != '\t' && *line != '\n' && *line != '|')
        {
            line++;
        }
    }

    // We mark the end of the argument list
    cmds[cmd_id][arg_id-1] = '\0';
    // We mark the end of the command list
    cmds[cmd_id+1][0] = '\0';
}

// This function will execute the command in a child process
void execute(char *cmds[MAX_NB_CMD][MAX_NB_ARG])
{
    pid_t pid;
    int pipefd[2];
    int last_out = fileno(stdin);
    int cmd_id;
    char buf;

    for(cmd_id=0; cmds[cmd_id][0] != NULL; cmd_id++) {

        if (pipe(pipefd) == -1)
        // We create a child process
        {
            fprintf(stderr, "Error: Cannot create a pipe\n");
            exit(EXIT_FAILURE);
        }
        else if ((pid = fork()) == -1)
        {
            fprintf(stderr, "Error: Cannot create a child process\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            // The process child execute the command
            if (pid == 0)
            {
                // We close the unused read end
                close(pipefd[0]);

                // We redirect the input to the pip
                dup2(last_out, fileno(stdin));

                // We redirect the output to the pip
                dup2(pipefd[1], fileno(stdout));

                if (execvp(*cmds[cmd_id], cmds[cmd_id]) == -1)
                {
                    fprintf(stderr, "Error: Failed to execute the command\n");
                    exit(EXIT_FAILURE);
                }

                // We close the write end
                close(pipefd[1]);

                // We stop the process
                exit(EXIT_SUCCESS);
            }
            // The process parent wait the child
            else
            {
                // We close the unused write end and read end
                close(pipefd[1]);
                // wait for the completion
                wait(NULL);

                // we keep the read end of the pipe for the next process
                last_out = pipefd[0];
            }
        }
    }

    // Finally, we show the file output
    while (read(pipefd[0], &buf, 1) == 1)
    {
        printf("%c", buf);
    }

    close(pipefd[0]);
}

int main(void)
{
    char line[1024];
    char *cmds[MAX_NB_CMD][MAX_NB_ARG];
    int i;

    while (1)
    {
        printf("> ");

        // We flush the output screen
        fflush(stdout);

        // We get the line
        fgets(line, sizeof(line), stdin);

        // We parse the input, to get arguments
        parse(line, cmds);

        // We verify if cmds got
        if (cmds[0][0] == NULL)
        {
            fprintf(stderr, "Error: Syntax error\n");
        }
        else if (strcmp(cmds[0][0], "exit") == 0)
            exit(EXIT_SUCCESS);
        
        // We execute
        execute(cmds);
    }
} 