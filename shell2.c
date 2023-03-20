#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>

int main() {
char command[1024];
char *token;
char *outfile;
char *errfile;
int i, fd,fd_err, amper, redirect, retid, status, piping, argc1;
int fildes[2];
char *argv[1000],  *argv2[1000];
char* prompt_str = "hello"; 
char memory[20][1024];
int location=0;
while (1)
{   
    
    printf("%s: ",prompt_str);
    fgets(command, 1024, stdin);
    command[strlen(command) - 1] = '\0';
    piping = 0;
    if((command[0]=='!') && (command[1]=='!') && (command[2]=='\0') ){
        for(int i=0; i<1024; i++){
            command[i]=memory[location-1][i];
        }
    }
    else{
        for(int i=0; i<1024; i++){
            memory[location][i]=command[i];
        }
        location=(location+1)%20;
    }

    /* parse command line */
    i = 0;
    token = strtok (command," ");
    while (token != NULL)
    {
        argv[i] = token;
        token = strtok (NULL, " ");
        i++;
        if (token && ! strcmp(token, "|")) {
            piping = 1;
            break;
        }
    }
    argv[i] = NULL;
    argc1 = i;

    /* Is command empty */
    if (argv[0] == NULL)
        continue;

    if (piping) {
        i = 0;
        while (token!= NULL)
        {
            token = strtok (NULL, " ");
            argv2[i] = token;
            i++;
        }
        argv2[i] = NULL;
    }


    if(!strcmp(argv[0],"quit")){ //allow user to exit
        printf("exiting the program\n");
        exit(0);
    }

    if (! strcmp(argv[i - 1], "!!")) {
        amper = 1;
        argv[i - 1] = NULL;
    }

    /* Does command line end with & */ 
    if (! strcmp(argv[i - 1], "&")) {
        amper = 1;
        argv[i - 1] = NULL;
    }
    else 
        amper = 0; 
    /* Changing directory*/ 
    if (!strcmp(argv[0], "cd")){
        chdir(argv[1]);
        continue;
    }
    if ((! strcmp(argv[0], "prompt")) && (argc1 == 3) && (! strcmp(argv[1], "="))) {
        prompt_str=argv[2];
        continue;
    }
    if(!strcmp(argv[0], "echo") &&  (argc1 == 2) && !strcmp(argv[1], "$?")) {
        printf("%d\n",status);
        continue;
    }
     if(!strcmp(argv[0], "echo")){
        for(int j = 1; argv[j]!=NULL ; j++)
        {
            printf("%s ",argv[j]);
        }
        printf("\n");
        continue;
    }

    //supporting a chain of redirection first error then output
    if ((argc1 > 3) && (! strcmp(argv[i - 4], ">")) && (! strcmp(argv[i - 2], "2>"))){
        redirect = 3;
        argv[i - 4] = NULL;
        outfile = argv[i - 3];
        errfile = argv[i - 1];
        }
    //supporting a chain of redirection first output then error
    else if ((argc1 > 3) && (! strcmp(argv[i - 4], "2>")) && (! strcmp(argv[i - 2], ">"))){
        redirect = 3;
        argv[i - 4] = NULL;
        outfile = argv[i - 1];
        errfile = argv[i - 3];
        }
    else if ((argc1 > 1) && ! strcmp(argv[i - 2], ">")) {
        redirect = 1;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else if ((argc1 > 1) && ! strcmp(argv[i - 2], "2>")) {
        redirect = 2;
        argv[i - 2] = NULL;
        errfile = argv[i - 1];
        }
    else if ((argc1 > 1) && ! strcmp(argv[i - 2], ">>")) {
        redirect = 4;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else 
        redirect = 0; 
    
    /* for commands not part of the shell command language */ 

    if (fork() == 0) { 
        /* redirection of IO ? */
        if (redirect==1) {
            fd = creat(outfile, 0660); 
            close (STDOUT_FILENO) ; 
            dup(fd); 
            close(fd); 
            /* stdout is now redirected */
        }
        /* redirection of ERR */
        if (redirect==2) {
            fd = creat(errfile, 0660); 
            close (STDERR_FILENO) ; 
            dup(fd); 
            close(fd); 
            /* stderr is now redirected */
        }  
        if (redirect==3) {
            fd_err = creat(errfile, 0660); 
            close (STDERR_FILENO) ; 
            dup(fd_err); 
            close(fd_err); 
            fd = creat(outfile, 0660); 
            close (STDOUT_FILENO) ; 
            dup(fd); 
            close(fd); 
        }  
        if (redirect==4) {
            fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
            close (STDOUT_FILENO) ; 
            dup(fd); 
            close(fd); 
            /* stdout is now redirected and appended to outfile*/
        }
        if (piping) {
            pipe (fildes);
            if (fork() == 0) { 
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
        else{
            execvp(argv[0], argv);
        }        
    }
    /* parent continues here */
    if (amper == 0)
        retid = wait(&status);
}
}

