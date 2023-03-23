#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <sys/types.h>

char *prompt_str = "hello";

void cHandler(int dummy)
{
    printf("You typed Control-C!\n");
    printf("%s: ", prompt_str);
    fflush(stdout);
}

int main()
{
    signal(SIGINT, cHandler);

    char command[1024], command2[1024];
    char *token;
    char *outfile;
    char *errfile;
    int i, fd, fd_err, amper, redirect, retid, status, piping, argc1;
    int fildes[2];
    char *argv[1000], *argv2[1000];
    char *arg[100][1000];
    char *words[1000];
    char keys[1000][1000], values[1000][1000];
    char memory[20][1024];
    int location = 0, map_index = 0;

    while (1)
    {
        printf("%s: ", prompt_str);

        fgets(command2, 1024, stdin);
        command2[strlen(command2) - 1] = '\0';

        // everytime he presses up or down i present the previous/next command. once he presses enter i execute that command and exit the arrow mode
        int counter = location;
        while (command2[0] == '\033')
        {
                    // system ("/bin/stty raw");

        // system ("/bin/stty cooked");
            switch (command2[2])
            { // the real value
            case 'A':
                // code for arrow up
                counter = (counter+19) % 20;
                //maybe add if counter= location that we went through all 20 commands

                if (memory[counter][0] == '\0')
                { // reached the downmost command - stay on same previous command
                    counter = (counter+1) % 20;
                }
                //present to the user the command which he navigated to
                printf("%s\n", memory[counter]);
                break;
            case 'B':
                // code for arrow down
                counter = (counter + 1) % 20;
                //maybe add if counter= location that we went through all 20 commands

                if (memory[counter][0] == '\0')
                { // reached the downmost command - stay on same previous command
                    counter = (counter+19) % 20;
                }
                //present to the user the command which he navigated to
                printf("%s\n", memory[counter]);
                break;
            }
            fgets(command2, 1024, stdin);
            if (command2[0] == '\n')//allow the user to press enter and choose the command we presented to him
            {
                for (int i = 0; i < 1024; i++)
                {
                    command2[i] = memory[counter][i];
                }
                break;
            }
            else //user pressed another arrow -  continue to next iteration
            {
                command2[strlen(command2) - 1] = '\0';//(need this in else otherwise seg fault)
            }
        }
        if(command2[0] == '\0'){
            printf("empty command\n");
            continue;
        }
             
        i = 0;
        words[i++] = strtok(command2, " ");
        while ((words[i] = strtok(NULL, " ")) != NULL)
            i++;

        if (words[0][0] == '$' && (i == 3) && (!strcmp(words[1], "=")))
        {
            memcpy(keys[map_index], words[0], strlen(words[0]));
            memcpy(values[map_index++], words[2], strlen(words[2]));
            continue;
        }

        for (int j = 0; j < i; j++)
        {
            if (words[j][0] == '$' && words[j][1] != '?')
            {
                for (int k = map_index; k >= 0; k--)
                {
                    if (!strcmp(words[j], keys[k]))
                    {
                        words[j] = values[k];
                        break;
                    }
                }
            }
        }

        int c = 0;
        for (int j = 0; j < i; j++)
        {
            for (int k = 0; k < strlen(words[j]); k++)
            {
                command[c++] = words[j][k];
            }
            if (j != i - 1)
            {
                command[c++] = ' ';
            }
        }
        command[c] = '\0';

        piping = 0;
        if ((command[0] == '!') && (command[1] == '!') && (command[2] == '\0'))
        {
            for (int i = 0; i < 1024; i++)
            {
                command[i] = memory[location - 1][i];
            }
        }
        else
        {
            for (int i = 0; i < 1024; i++)
            {
                memory[location][i] = command[i];
            }
            location = (location + 1) % 20;
        }

        // piping !!!!!!!!!!!!!!!!!!!!!
        // for (int i = 0; i < 1024; i++)
        // {
        //     if (command[i]=='|'){
        //         piping=1;
        //         break;
        //     }
        // }
        // if(piping){

        //     i = 0;
        //     token = strtok (command,"|");
        //     char* args_pipe[100];
        //     while (token != NULL)
        //     {
        //         args_pipe[i] = token;
        //         token = strtok (NULL, "|");
        //         i++;
        //     }
        //     args_pipe[i] = NULL;

        //     for (int j = 0; j < i; j++){
        //         int k=0;
        //         token = strtok (args_pipe[j]," ");
        //         while (token != NULL)
        //         {
        //             arg[j][k] = token;
        //             token = strtok (NULL, " ");
        //             k++;
        //         }
        //         arg[j][k] = NULL;
        //     }

        // }

        // else{

        // }
        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
            if (token && !strcmp(token, "|"))
            {
                piping = 1;
                break;
            }
        }
        argv[i] = NULL;
        argc1 = i;

        /* Is command empty */
        if (argv[0] == NULL)
            continue;

        if (piping)
        {
            i = 0;
            while (token != NULL)
            {
                token = strtok(NULL, " ");
                argv2[i] = token;
                i++;
            }
            argv2[i] = NULL;
        }

        if (!strcmp(argv[0], "quit"))
        { // allow user to exit
            printf("exiting the program\n");
            exit(0);
        }

        if (!strcmp(argv[argc1 - 1], "!!"))
        {
            amper = 1;
            argv[argc1 - 1] = NULL;
        }

        /* Does command line end with & */
        if (!strcmp(argv[argc1 - 1], "&"))
        {
            amper = 1;
            argv[argc1 - 1] = NULL;
        }
        else
            amper = 0;
        /* Changing directory*/
        if (!strcmp(argv[0], "cd"))
        {
            chdir(argv[1]);
            continue;
        }

        if ((!strcmp(argv[0], "prompt")) && (argc1 == 3) && (!strcmp(argv[1], "=")))
        {
            prompt_str = argv[2];
            continue;
        }
        if (!strcmp(argv[0], "echo") && (argc1 == 2) && !strcmp(argv[1], "$?"))
        {
            printf("%d\n", status);
            continue;
        }
        if (!strcmp(argv[0], "echo"))
        {
            for (int j = 1; argv[j] != NULL; j++)
            {
                printf("%s ", argv[j]);
            }
            printf("\n");
            continue;
        }

        // supporting a chain of redirection first error then output
        if ((argc1 > 3) && (!strcmp(argv[argc1 - 4], ">")) && (!strcmp(argv[argc1 - 2], "2>")))
        {
            redirect = 3;
            argv[argc1 - 4] = NULL;
            outfile = argv[argc1 - 3];
            errfile = argv[argc1 - 1];
        }
        // supporting a chain of redirection first output then error
        else if ((argc1 > 3) && (!strcmp(argv[argc1 - 4], "2>")) && (!strcmp(argv[argc1 - 2], ">")))
        {
            redirect = 3;
            argv[argc1 - 4] = NULL;
            outfile = argv[argc1 - 1];
            errfile = argv[argc1 - 3];
        }
        else if ((argc1 > 1) && !strcmp(argv[argc1 - 2], ">"))
        {
            redirect = 1;
            argv[argc1 - 2] = NULL;
            outfile = argv[argc1 - 1];
        }
        else if ((argc1 > 1) && !strcmp(argv[argc1 - 2], "2>"))
        {
            redirect = 2;
            argv[argc1 - 2] = NULL;
            errfile = argv[argc1 - 1];
        }
        else if ((argc1 > 1) && !strcmp(argv[argc1 - 2], ">>"))
        {
            redirect = 4;
            argv[argc1 - 2] = NULL;
            outfile = argv[argc1 - 1];
        }
        else
            redirect = 0;

        /* for commands not part of the shell command language */

        if (fork() == 0)
        {
            /* redirection of IO ? */
            if (redirect == 1)
            {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            /* redirection of ERR */
            if (redirect == 2)
            {
                fd = creat(errfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
                /* stderr is now redirected */
            }
            if (redirect == 3)
            {
                fd_err = creat(errfile, 0660);
                close(STDERR_FILENO);
                dup(fd_err);
                close(fd_err);
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            if (redirect == 4)
            {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected and appended to outfile*/
            }
            if (piping)
            {
                pipe(fildes);
                if (fork() == 0)
                {
                    /* first component of command line */
                    close(STDOUT_FILENO);
                    dup(fildes[1]);
                    close(fildes[1]);
                    close(fildes[0]);
                    /* stdout now goes to pipe */
                    /* child process does command */
                    execvp(argv[0], argv);
                }
                /* 2nd command component of command line */
                close(STDIN_FILENO);
                dup(fildes[0]);
                close(fildes[0]);
                close(fildes[1]);
                /* standard input now comes from pipe */
                execvp(argv2[0], argv2);
            }
            else
            {
                execvp(argv[0], argv);
                exit(1);
            }
        }
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
    }
}
