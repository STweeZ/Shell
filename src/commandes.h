#include "global.h"
#include <glob.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <assert.h>

typedef struct Variable Variable;

struct Variable {
  char * name;
  char * val;
  Variable *precedent;
  Variable *suivant;
};

typedef struct Liste Liste;

struct Liste {
    Variable *variable;
};

typedef enum { RUNNING = 10, STOPPED = 11, OVER = 12 } Etat;

typedef struct Job Job;

struct Job {
  char * cmd;
  pid_t pid;
  int jobValue;
  Etat state;
  int retour;
  Job *precedent;
  Job *suivant;
};

typedef struct Jobs Jobs;

struct Jobs {
    Job *job;
};

void mycd(char * directory);

void myls(char * directory, char * parameters);

int myglob(glob_t globbuf, char * tab[], int limit);

void myexit();

int setLocalVariable(char * infos, Liste *liste);

int manageVariables(int p[2], char * tab[], int size, Liste *liste);

void allVariables(Liste *liste);

int isVariable(char * val);

char *valVariable(char * var, Liste *liste);

void freeVariables(Liste *liste);

int variableExists(char * name, Liste *liste);

int unsetVariable(char * name, Liste *liste);

void getAllJobs(Jobs *liste);

void freeJob(Jobs *liste);

int setJob(char * command, pid_t pid, Jobs *liste, Etat state);

int unsetJob(pid_t pid, Jobs *liste);

void printJob(Job *job);

void killJobs(Jobs *liste);