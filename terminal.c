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

#define MAX_NB_ARG 32
#define MAX_NB_CMD 32
#define MAX_ARG_LENGTH 1024
#define DEBUG 0

/* This function split a line in many parts delimited by a space
   "cat < a | grep .git/ref | sort | more > b" will become "cat < a | grep ter | sort | more > b"
                                                   MODE ID |   |0| |1|        |2|    |3|    |4| |
                                                           |------------------------------------|
                                                   CMD ID  |    0   |     1    |   2  |    3    |
                                                           |------------------------------------|
                                                   ARG ID  | 0 |  1 |  0  | 1  |   0  |  0  | 1 |
 
    "cat < a > b" will become "cat < a > b"
                      MODE ID |   |0| |1| |
                              |-----------|
                      CMD ID  |     0     |
                              |-----------|
                      ARG ID  | 0 | 1 |  2|

    "echo beginning... & dir > a || ls > a && echo finished" will become "echo beginning... & dir > a || ls > a && echo finished"
                                                                MODE ID  |       0          |    |0|  ||   |0|  ||              |
                                                                         |------------------|---------||--------||--------------|
                                                                CMD ID   |       0          |    0    ||    0   ||      0       |
                                                                         |------------------|---------||--------||--------------|
                                                                ARG ID   | 0  |     1       | 0  | 1  || 0 | 1  ||  0  |   1    |
                                                                         |------------------|-----------------------------------|
                                                                PROC ID  |        1         |                0                  |
*/
void parse(char* line, char* cmds[MAX_NB_CMD][MAX_NB_ARG], char* modes)
{
    int cmd_id = 0;
    int arg_id = 0;

    // the first mode is always null
    *modes++ = '\0';

    while (*line != '\0')
    {
        // We mark the end of an argument
        while (*line == ' ' || *line == '\t' || *line == '\n' || *line == '|' || *line == '<' || *line == '>')
        {
            // We search an other command
            switch (*line)
            {
                case '|':
                case '<':
                case '>':
                    // We end the argument
                    cmds[cmd_id++][arg_id] = '\0';
                    arg_id = 0;
                    // We config the mode
                    *modes++ = *line;
                    break;
                default:
                    break;
            }

            // We mark the end the mode
            *modes = '\0';

            // We mark the end of this point of the line and go to the next
            *line++ = '\0';
        }

        // This argument will begin at this point of the line
        // we mark the begin and go to the next argument
        cmds[cmd_id][arg_id++] = line;

        // We skip the other part of the argument
        while (*line != ' ' && *line != '\0' && *line != '\t' && *line != '\n' && *line != '|' && *line != '<' && *line != '>')
            line++;
    }

    // We mark the end of the argument list
    // We verify if the line end by a empty character
    if (*(line-1) == 0)
        cmds[cmd_id][arg_id-1] = '\0';

    // We mark the end of the command list
    cmds[cmd_id+1][0] = '\0';
}

/*
    This function will execute the command in a child process
    return -1 in case of error
*/
int execute(char* cmds[MAX_NB_CMD][MAX_NB_ARG], char* modes)
{
    pid_t pid;
    int pipefd[2];

    // an error will be detected in reading this pipe
    int errorfd[2];
    
    // default input/output
    int din = fileno(stdin);
    int dout = fileno(stdout);
    
    // current input/output
    int in = din;
    int out = dout;
    
    // old output
    int old_output = in;
    
    // current command
    int cmd_id;
    char buf=0;

    for(cmd_id=0; cmds[cmd_id][0] != NULL; cmd_id++) {
        // debug
        if (DEBUG)
        {
            fprintf(stderr, "DEBUG: ");

            for(int i=0; cmds[cmd_id][i] != NULL; i++)
                fprintf(stderr, "%s ", cmds[cmd_id][i]);
            
            fprintf(stderr, ":%c %c:\n", modes[cmd_id], modes[cmd_id+1]);
        }

        // We verify if the current command is a normal command
        switch (modes[cmd_id])
        {
            case '<':
            case '>':
                continue;
            default:
                break;
        }

        if (pipe(pipefd) == -1 || pipe(errorfd) == -1)
        // We create a child process
        {
            fprintf(stderr, "Error: Cannot create a pipe\n");
            _exit(EXIT_FAILURE);
        }
        else if ((pid = fork()) == -1)
        {
            fprintf(stderr, "Error: Cannot create a child process\n");
            _exit(EXIT_FAILURE);
        }
        else
        {
            // The process child execute the command
            if (pid == 0)
            {
                // We close the unused read end
                close(pipefd[0]);
                close(errorfd[0]);

                // We redirect the stderr to the error pipe
                dup2(errorfd[1], fileno(stderr));

                // We config the input and output in function of the mode
                switch (modes[cmd_id+1])
                {
                    // ls | more
                    case '|':
                        // We redirect the output to the pipe
                        out = pipefd[1];
                        break;
                    // cat < foo.txt
                    // cat < foo.txt > foo2.txt
                    // cat < foo.txt | sort
                    case '<':
                        // We redirect the input to a file
                        in = fileno(fopen(*cmds[cmd_id+1], "rb"));

                        // We redirect the output
                        // to a file
                        if (modes[cmd_id+2] == '>')
                        {
                            out = fileno(fopen(*cmds[cmd_id+2], "wb"));
                        }
                        // to the pipe
                        else if (modes[cmd_id+2] == '|')
                        {
                            out = pipefd[1];
                        }
                        // to the normal output
                        else {
                            out = out;
                        };
                        break;
                    // ls > foo.txt
                    case '>':
                        // We redirect the output to a file
                        out = fileno(fopen(*cmds[cmd_id+1], "wb"));
                        break;
                }

                // ls | more
                // We redirect the input to the old output
                if (modes[cmd_id] == '|')
                {
                    in = old_output;
                }

                // We apply the redirection
                dup2(in, 0);
                dup2(out, 1);

                if (execvp(*cmds[cmd_id], cmds[cmd_id]) == -1)
                {
                    fprintf(stderr, "Error: Failed to execute the command\n");
                    _exit(EXIT_FAILURE);
                }

                // We close the write end
                close(pipefd[1]);
                close(errorfd[1]);

                // We close the input
                close(fileno(stdin));

                // We close the output
                close(fileno(stdout));

                // We stop the child process
                _exit(EXIT_SUCCESS);
            }
            // The process parent wait the child
            else
            {
                // We close the unused write end and read end
                close(pipefd[1]);
                close(errorfd[1]);

                // wait for the completion
                wait(NULL);

                // We verify if we have an error
                while (read(errorfd[0], &buf, 1) == 1)
                    printf("%c", buf);

                // If an error is got
                // We stop the executing
                if (buf)
                    break;

                // we keep the read end of the pipe for the next process
                old_output = pipefd[0];
            }
        }
    }

    // We close unused read end
    close(pipefd[0]);
    close(errorfd[0]);

    // We return an error status
    return (buf == 0) ? 0 : -1;
}

void eval(char* line)
{
    char* cmds[MAX_NB_CMD][MAX_NB_ARG];

    // Represent the mode of operation of each command
    char modes[MAX_NB_CMD];

    // represent a point in the line where a command begin
    char *pline = line;

    // represent the execution mode
    // last and current
    char exec_mode[2] = "";

    pid_t pid;

    int last_exec_status = 0;

    while (*line++ != '\0')
    {
        if (*line == '&')
        {
            // we set parallel execution
            exec_mode[1] = 'p';

            *line = '\0';
            if (*(line+1) == '&')
            {
                // we set "and" execution
                exec_mode[1] = 'a';

                *(++line) = '\0';
            }
        }
        else if (*line == '|' && *(line+1) == '|')
        {
            // we set "or" execution
            exec_mode[1] = 'o';

            *line = '\0';
            *(++line) = '\0';
        }
        else if (*(line+1) == '\0')
        {
            // we set normal execution
            exec_mode[1] = '\0';
        }
        else
        {
            // We keep the next instructions
            continue;
        }

        // We parse the input, to get arguments
        parse(pline, cmds, modes);

        // We verify if cmds got
        if (cmds[0][0] == NULL)
        {
            ;//
        }
        else if (strcmp(cmds[0][0], "exit") == 0)
        {
            _exit(EXIT_SUCCESS);
        }
        else
        {
            // We execute the commands in function of the execution mode
            // ls & dir

            if (exec_mode[1] == 'p')
            {
                if (DEBUG)
                    fprintf(stderr, "DEBUG: exec_mode in child %c\n", exec_mode[1]);
                
                if ((pid=fork()) == -1)
                {
                    fprintf(stderr, "Error: Cannot create a child process\n");
                    _exit(EXIT_FAILURE);
                }
                else
                {
                    if (pid == 0)
                    {
                        // we terminate the child process
                        _exit((execute(cmds, modes) == -1) ? EXIT_FAILURE : EXIT_SUCCESS);
                    }
                }
            }
            // ls || dir
            // ls && dir
            // ls
            else
            {
                if (DEBUG)
                    fprintf(stderr, "DEBUG: exec_mode %c\n", exec_mode[0]);
                
                if (exec_mode[0] == 'o' & last_exec_status == -1 || exec_mode[0] == 'a' & last_exec_status != -1 || exec_mode[0] == '\0' || exec_mode[0] == 'p')
                    last_exec_status = execute(cmds, modes);
            }
        }
        
        // we save the last execution mode
        exec_mode[0] = exec_mode[1];
        pline = ++line;
    }
}

int main(void)
{
    char line[1024];
    
    while (1)
    {
        printf("> ");

        // We flush the output screen
        fflush(stdout);

        // We get the line
        fgets(line, sizeof(line), stdin);

        // We evaluate
        eval(line);
    }
}