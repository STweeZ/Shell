#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define ERR -1
#define FAILED_EXEC 127
#define LGCMD_SIZE 4096 
#define BUFFER_SIZE 1024
#define MAX 200

#define clear() printf("\e[1;1H\e[2J")
#define syserror(x) perror(errormsg[x])
#define fatalsyserror(x) syserror(x), exit (x)
#define err(m,n) printf("Error %c",m), exit(n)
#define ROUGE(m) "\033[01;31m"m"\033[0m"
#define YELLOW(m) "\033[01;34m"m"\033[0m"
#define BLUE(m) "\033[01;34m"m"\033[0m"
#define CYAN(m) "\033[01;36m"m"\033[0m"
#define VERT(m) "\033[01;32m"m"\033[0m"
