#include "redirections.h"

bool redirect(char ** tabcmd, int size) {
    bool redir = false;
    char * type_redir;
    char * src[100];
    char * dest;

    // we search for a pipe
    for (int i = 0; i < size; i++) {
                
        if (strstr(tabcmd[i], ">") || strstr(tabcmd[i], ">>")/* TO DO add other redir*/)  {
            type_redir = tabcmd[i];
            redir = true;
        }
        else if (!redir) src[i] = tabcmd[i];
        else dest = tabcmd[i];
    }

    if (redir) {
        // 0 is read, 1 is write    
        int pipefd[2];
        char buffer[4096];

        if (pipe(pipefd) < 0) {
            printf("Pipe could not be initialized\n");
            exit(FAILED_EXEC);
        }
        
        if (strstr(type_redir, ">>")) {

            if (fork() == 0) {
                int fd = open(dest, O_RDWR | O_APPEND | S_IWUSR);

                if (strstr(type_redir, "2")) {
                    dup2(fd, STDERR_FILENO);
                }
                else if (strstr(type_redir, "&")) {
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                }
                else {
                    // write the result of exec in the file
                    dup2(fd, STDOUT_FILENO);
                }
                close(fd);

                execvp(src[0], src);
            }
            else {
                // wait for the child
                wait(NULL);
            }
        }
        else if (strstr(type_redir, ">")) {

            if (fork() == 0) {
                int fd = open(dest, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

                if (strstr(type_redir, "2")) {
                    dup2(fd, STDERR_FILENO);
                }
                else if (strstr(type_redir, "&")) {
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                }
                else {
                    // write the result of exec in the file
                    dup2(fd, STDOUT_FILENO);
                }
                close(fd);

                execvp(src[0], src);
            }
            else {
                // wait for the child
                wait(NULL);
            }
        }
    }
    return redir;
}

// function where the piped system commands is executed
void pipedExec(char** parsed, char** parsedpipe, int pipe_size)
{

    // 0 is read, 1 is write
    int pipefd[2]; 
    pid_t p1, p2, p3;
  
    if (pipe(pipefd) < 0) {
        printf("Pipe could not be initialized\n");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("Could not fork\n");
        return;
    }
  
    if (p1 == 0) {
        // child 1
        // it only needs to write
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
  
        execvp(parsed[0], parsed);
        exit(FAILED_EXEC);
    } 
    else {
        // father

        bool piped = true; // start as true to execute the while once
        char * str_piped[100];
        char * str[100];
        int size;
        int c = 1;
        while (piped) {
            c++;
            if ((size = isPiped(parsedpipe, str_piped, pipe_size, str)) > 0) {
                // printf("more pipe\n");

                p3 = fork();

                if (p3 < 0) {
                    printf("Could not fork\n");
                    return;
                }

                // child
                // it needs to read and write
                if (p3 == 0) {
                    // printf("infinite pipe");
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);

                    // printf("infinite pipe exec");
                    execvp(str[0], str);
                    exit(FAILED_EXEC);
                }

                for (int v = 0; v < pipe_size; v++) {
                    if (v < size) parsedpipe[v] = str_piped[v];
                    else parsedpipe[v] = NULL;
                }
                pipe_size = size;
            } else {
                // printf("no more pipe\n");
                piped = false;

                p2 = fork();
        
                if (p2 < 0) {
                    printf("Could not fork\n");
                    return;
                }

                // child
                // it only needs to read
                if (p2 == 0) {
                    // printf("child p2\n");
                    close(pipefd[1]);
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]);

                    // printf("child exec\n");
                    execvp(str[0], str);
                    exit(FAILED_EXEC);
                }
                else {
                    // close the pipe for the father or the childs will wait indefinitely for EOF in the pipe
                    close(pipefd[1]);
                    close(pipefd[0]);
                    // printf("father's waiting...\n");
                    // parent executing, waiting for childrens
                    for (int v = 0; v < c; v++) {
                        // printf("wait...\n");
                        wait(NULL);
                    }
                    exit(0);
                }
            }
        }
    }
}

// check if str contain a pipe and store it in str_piped and str
int isPiped(char ** tabcmd, char** str_piped, int cmd_size, char ** str)
{
    int i;
    int c = 0;
    int n = 0;
    bool pipe = false;
    // we search for a pipe
    for (i = 0; i < cmd_size; i++) {
        if (strstr(tabcmd[i], "|") && !pipe)
            pipe = true;
        else if (pipe) {
            // we copy the content after the pipe
            str_piped[c++] = tabcmd[i];
        }
        else {
            str[n++] = tabcmd[i];
        }
    }
    if (pipe)
        return c; // return the size if a pipe was found
    else
        return 0;
}