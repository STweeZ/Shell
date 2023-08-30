#include "commandes.h"

struct dirent *ptr;

void mycd(char * directory) {
  if (strcmp(directory, "~") == 0) directory = getenv("HOME");
    if (chdir(directory) != 0)
        printf("No such file or directory\n");
}

void myexit() {
  exit(0);
}

/**
 * @brief
 *
 * @param mode
 * @param str
 */
void modeToLetter(int mode, char *str) {
  str[0] = '-';

  // file type
  if (S_ISDIR(mode))
    str[0] = 'd';

  if (S_ISCHR(mode))
    str[0] = 'c';

  if (S_ISBLK(mode))
    str[0] = 'b';

  // permission for owner
  if (mode & S_IRUSR)
    str[1] = 'r';
  else
    str[1] = '-';

  if (mode & S_IWUSR)
    str[2] = 'w';
  else
    str[2] = '-';

  if (mode & S_IXUSR)
    str[3] = 'x';
  else
    str[3] = '-';

  // permission for owner group
  if (mode & S_IRGRP)
    str[4] = 'r';
  else
    str[4] = '-';

  if (mode & S_IWGRP)
    str[5] = 'w';
  else
    str[5] = '-';

  if (mode & S_IXGRP)
    str[6] = 'x';
  else
    str[6] = '-';

  // permission for others
  if (mode & S_IROTH)
    str[7] = 'r';
  else
    str[7] = '-';

  if (mode & S_IWOTH)
    str[8] = 'w';
  else
    str[8] = '-';

  if (mode & S_IXOTH)
    str[9] = 'x';
  else
    str[9] = '-';

  str[10] = '\0';
} // modeToLetter

/**
 * @brief
 *
 * @param direct
 * @return int
 */
int maxSizeInRep(char *direct, int * totalblocks) {
  long int max = 0;
  struct stat fst;
  char * buffer = NULL;
  int sizeBuffer = 0;
  DIR *dir = opendir(direct);

  if (!dir) {
    perror("opendir");
    exit(1);
  }

  while ((ptr = readdir(dir)) != NULL) {

    if ((!strncmp(ptr->d_name, ".", 1) || !strcmp(ptr->d_name, "..")))
      continue; // skip . and ..

    if (strlen(direct) + strlen(ptr->d_name) + 1 >= sizeBuffer) {
      sizeBuffer = strlen(direct) + strlen(ptr->d_name) + 10;
      buffer = (char *)realloc(buffer, sizeBuffer);
      assert(buffer);
    }

    sprintf(buffer, "%s", ptr->d_name);
    if (fstatat(dirfd(dir), buffer, &fst, AT_SYMLINK_NOFOLLOW) == -1) {
      perror("fstatat maxSizeInRep");
      exit(1);
    }

    if ((long)fst.st_size > max)
      max = (long)fst.st_size;

    *totalblocks = *totalblocks + fst.st_blocks;
  }

  if (closedir(dir) == -1) {
    perror("close dir");
    exit(1);
  }

  if (buffer)
    free(buffer);
  return max;
} // maxSizeInRep


void myls(char * directory, char * parameters) {
    if (strcmp(directory, "") == 0) directory = ".";
    else if (strcmp(directory, "~") == 0) directory = getenv("HOME");
    // printf("x%sx\n", directory);
    // printf("x%sx\n", parameters);
    struct stat fst;
    struct tm *mytime;
    char str[12];
    // printf("myls1\n");
    int totalblocks = 0;
    long int maxSize = maxSizeInRep(directory, &totalblocks);
    // printf("myls2\n");
    unsigned length = floor(log10(maxSize)) + 1;
    char *buffer = NULL;
    int sizeBuffer = 0;
    int count = 0;
    bool isR = strstr(parameters, "R") != NULL;

    if (isR) {
      if (stat(directory, &fst) == -1) {
        perror("stat");
        exit(1);
      }

      if (!S_ISDIR(fst.st_mode))
        printf("%s\n", directory);
      else
        printf("%s:\n", directory);
    }
    
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("opendir");
        exit(1);
    }

    printf("total %d\n", totalblocks/2);

    while ((ptr = readdir(dir)) != NULL) {
        if ((!strncmp(ptr->d_name, ".", 1) || !strcmp(ptr->d_name, "..")) && strstr(parameters, "a") == NULL)
        continue; // skip anything that start with . and .. except if a is in the parameters

        if (strlen(directory) + strlen(ptr->d_name) + 1 >= sizeBuffer) {
        sizeBuffer = strlen(directory) + strlen(ptr->d_name) + 10;
        buffer = (char *)realloc(buffer, sizeBuffer);
        assert(buffer);
        }

        sprintf(buffer, "%s", ptr->d_name);
        if (fstatat(dirfd(dir), buffer, &fst, AT_SYMLINK_NOFOLLOW) == -1) {
        perror("fstatat");
        exit(1);
        }

        modeToLetter(fst.st_mode, str);               // permission information
        printf("%s", str);                            // file type and permission
        printf(" %ld", fst.st_nlink);                 // file hard links
        printf(" %s", getpwuid(fst.st_uid)->pw_name); // file's owner
        printf(" %s", getgrgid(fst.st_gid)->gr_name); // file's owner group
        printf(" %*ld", length, (long)fst.st_size);   // file size
        mytime = localtime(&fst.st_mtime);            // file time
        printf(" %d-%02d-%02d %02d:%02d", mytime->tm_year + 1900,
            mytime->tm_mon + 1, mytime->tm_mday, mytime->tm_hour,
            mytime->tm_min);
        if (S_ISDIR(fst.st_mode)) printf(BLUE(" %s"), ptr->d_name); // if is directory write the name in blue
        else if (S_ISLNK(fst.st_mode)) printf(CYAN(" %s"), ptr->d_name); // if is symbolic link write the name in cyan
        else if (S_ISBLK(fst.st_mode) || S_ISCHR(fst.st_mode)) printf(YELLOW(" %s"), ptr->d_name); // if is block device or character device write the name in yellow
        // else is regular file
        else printf(" %s", ptr->d_name); // file name
        printf("\n");
    }

    // le option_R du prof marche pas 
    if (isR) {
      printf("\n");

      // recursive call
      rewinddir(dir);

      while ((ptr = readdir(dir)) != NULL) {

        if ((!strncmp(ptr->d_name, ".", 1) || !strcmp(ptr->d_name, "..")))
          continue; // skip . and ..

        // if directory
        if (ptr->d_type & DT_DIR) {

          if (strlen(directory) + strlen(ptr->d_name) + 1 >= sizeBuffer) {
            sizeBuffer = strlen(directory) + strlen(ptr->d_name) + 10;
            buffer = (char *)realloc(buffer, sizeBuffer);
            assert(buffer);
          }

          sprintf(buffer, "%s/%s", directory, ptr->d_name);

          myls(buffer, parameters);
        }
      }
    }

    if (closedir(dir) == -1) {
        perror("close dir");
        exit(1);
    }

    if (buffer)
        free(buffer);
}

int is_joker(char c) { // the instruction is a wildcard
  if (c == '*' || c == '?' || c == '[') return 1;
  return 0;
}

int myglob(glob_t globbuf, char *tab[BUFFER_SIZE], int limit) {
  int count=0, joker=0, index[100], count2=0, jokers[100];
  for(int x=0; x<limit; x++) { // check if there is wildcards in instruction
    for(int y=0; y<strlen(tab[x]); y++) if(is_joker(tab[x][y])) {
      joker=1;
      break;
    }
    if(joker) jokers[count2++] = x; // jokers to manage
    else index[count++] = x;
    joker=0;
  }
  globbuf.gl_offs = count; // set empty space at the beginning of the array for the command
  for(int x=0; x<count2; x++) { // manage jokers
    if(x==0) glob(tab[jokers[x]], GLOB_DOOFFS, NULL, &globbuf);
    else glob(tab[jokers[x]], GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
  }
  if(count2) {
    if(!globbuf.gl_pathc) {
      globfree(&globbuf);
      return 0;
    }
    for(int x=0; x<count; x++) globbuf.gl_pathv[x] = tab[index[x]]; // put command into empty space
    execvp(globbuf.gl_pathv[0], &globbuf.gl_pathv[0]); // execute the command with wildcards
  }
  return 1;
}

int isVariable(char * val) { // check if we call a variable
  return *val == '$';
}

char *valVariable(char * var, Liste *liste) { // get the value of a variable
  if(liste == NULL) exit(EXIT_FAILURE);
  var++;
  Variable *v = liste->variable;
  while(v!=NULL) {
    if(strcmp(var,v->name) == 0) return v->val;
    v=v->suivant;
  }
}

void allVariables(Liste *liste) { // print all local variables
  if(liste == NULL) exit(EXIT_FAILURE);
  Variable *v = liste->variable;
  if(v == NULL) return;
  printf("Local variables :\n");
  while(v!=NULL) {
    printf("%s=%s\n",v->name,v->val);
    v=v->suivant;
  }
}

void freeVariables(Liste *liste) { // free variables
  if(liste == NULL) exit(EXIT_FAILURE);
  Variable *v = liste->variable;
  if(v != NULL) {
    while(v->suivant) {
      v=v->suivant;
      free(v->precedent->name);
      free(v->precedent->val);
      free(v->precedent);
    }
    free(v->name);
    free(v->val);
    free(v);
  }
  free(liste);
  return;
}

int variableExists(char * name, Liste *liste) { // check if the variable with this name exists
  if(liste == NULL) exit(EXIT_FAILURE);
  Variable *v = liste->variable;
  while(v!=NULL) {
    if(strcmp(name,v->name) == 0) return 1;
    v=v->suivant;
  }
  return 0;
}

int setLocalVariable(char * infos, Liste *liste) { // set a local variable
  char *name = malloc(sizeof(char)* MAX);
  char *value = malloc(sizeof(char)* MAX);
  int x=0, y=0;
  while(x<strlen(infos) && infos[x]!='=') name[y++] = infos[x++];
  name[y]='\0';
  y=0;
  x++;
  while(x<strlen(infos)) value[y++] = infos[x++];
  value[y]='\0';

  if(variableExists(name,liste)) { // check if the variable with this name exists
    free(name);
    free(value);
    return 1;
  }

  if(isVariable(value)) { // check if we want to affect a variable with the value of an other variable
    char *tempVar = valVariable(value, liste);
    if(!tempVar) {
      free(name);
      free(value);
      return 1;
    }
    else strcpy(value,tempVar);
  }
  Variable *lVar = malloc(sizeof(*lVar));
  if(liste == NULL || lVar == NULL) exit(EXIT_FAILURE);
  lVar->name = malloc(MAX*sizeof(char));
  strcpy(lVar->name, name);
  free(name);
  lVar->val = malloc(MAX*sizeof(char));
  strcpy(lVar->val, value);
  free(value);
  lVar->suivant = liste->variable;
  lVar->precedent = NULL;
  if(liste->variable) liste->variable->precedent = lVar; // add the variable to the list of existing variables
  liste->variable = lVar;
  return 0;
}

int unsetVariable(char * name, Liste *liste) { // unset a variable = remove from the list of existing variables
  name++;
  if (liste == NULL) exit(EXIT_FAILURE);
  Variable *actuel = liste->variable;

  while (actuel != NULL) {
      if(strcmp(name,actuel->name)==0) break;
      actuel = actuel->suivant;
  }
  if(actuel != NULL) {
      if(actuel==liste->variable) {
          liste->variable = actuel->suivant;
          if(liste->variable) liste->variable->precedent=NULL;
      } else actuel->precedent->suivant = actuel->suivant;
      if(actuel->suivant) {
          actuel->suivant->precedent = actuel->precedent;
      }
      free(actuel->name);
      free(actuel->val);
      free(actuel);
  }
  return 0;
}

int manageVariables(int p[2], char * tab[], int size, Liste *liste) {
  close(p[0]);
  if(strcmp("set",*tab) == 0) {
    if(size == 1) { // we want to see the list of all variables
      allVariables(liste);
      close(p[1]);
      return 0;
    } else { // we want to set a local variable, we check if the informations given are correct
      char name[BUFFER_SIZE];
      int x=0, y=0;
      while(x<strlen(tab[1]) && tab[1][x]!='=') name[y++] = tab[1][x++];
      if(y==0) {
        close(p[1]);
        return 0;
      }
      y=0;
      x++;
      char value[BUFFER_SIZE];
      while(x<strlen(tab[1])) value[y++] = tab[1][x++];
      if(y==0) {
        close(p[1]);
        return 0;
      }
      write(p[1],tab[1],sizeof(char) * BUFFER_SIZE); // send informations from the pipe
      close(p[1]);
      return 47;
    }
  } else if(strcmp("unset",*tab) == 0) { // we want to unset a local variable
    if(size != 2) {
      close(p[1]);
      return 0;
    }
    write(p[1],tab[1],sizeof(char) * BUFFER_SIZE); // send informations from the pipe
    close(p[1]);
    return 57;
  }
  close(p[1]);
}

void getAllJobs(Jobs *liste) { // print all jobs
  if(liste == NULL) exit(EXIT_FAILURE);
  Job *j = liste->job;
  if(j == NULL) return;
  printf("Jobs :\n");
  while(j!=NULL) {
    if(j->state==10) printf("[%i] %d En cours d'exécution %s\n",j->jobValue,j->pid,j->cmd);
    else printf("[%i] %d Stoppé %s\n",j->jobValue,j->pid,j->cmd);
    j=j->suivant;
  }
}

void freeJob(Jobs *liste) { // free jobs
  if(liste == NULL) exit(EXIT_FAILURE);
  Job *j = liste->job;
  if(j != NULL) {
    while(j->suivant) {
      j=j->suivant;
      free(j->precedent->cmd);
      free(j->precedent);
    }
    free(j->cmd);
    free(j);
  }
  free(liste);
  return;
}

int setJob(char * command, pid_t pid, Jobs *liste, Etat state) { // set a job
  Job *newJob = malloc(sizeof(*newJob));
  if(liste == NULL || newJob == NULL) exit(EXIT_FAILURE);
  newJob->cmd = malloc(MAX*sizeof(char));
  strcpy(newJob->cmd, command);
  newJob->pid = pid;
  if(liste->job) newJob->jobValue = liste->job->jobValue+1;
  else newJob->jobValue = 1;
  newJob->state = state;
  newJob->suivant = liste->job;
  newJob->precedent = NULL;
  if(liste->job) liste->job->precedent = newJob; // add the job to the list of jobs
  liste->job = newJob;
  if(newJob->state == STOPPED) printf("%s [%d] %d Stopped\n",newJob->cmd,newJob->jobValue,newJob->pid);
  else printf("[%d] %d\n",newJob->jobValue,newJob->pid);
  return 0;
}

int unsetJob(pid_t pid, Jobs *liste) { // unset a job = remove from the list of current jobs
  if (liste == NULL) exit(EXIT_FAILURE);
  Job *actuel = liste->job;

  while (actuel != NULL) {
      if(actuel->pid == pid) break;
      actuel = actuel->suivant;
  }
  if(actuel != NULL) {
      if(actuel==liste->job) {
          liste->job = actuel->suivant;
          if(liste->job) liste->job->precedent=NULL;
      } else actuel->precedent->suivant = actuel->suivant;
      if(actuel->suivant) {
          actuel->suivant->precedent = actuel->precedent;
      }
      free(actuel->cmd);
      free(actuel);
  }
  return 0;
}

void printJob(Job *job) { // print specific job status
  if(!job) return;
  if(job->retour == 127) printf("%d terminé anormalement\n",job->pid);
  else if(job->state == STOPPED) printf("%d stoppé\n",job->pid);
  else printf("%d terminé avec comme code de retour %d\n",job->pid,job->retour);
}

void killJobs(Jobs *liste) { // kill all background jobs
  if(liste == NULL) exit(EXIT_FAILURE);
  Job *j = liste->job;
  if(j != NULL) {
    while(j->suivant) {
      j=j->suivant;
      kill(j->precedent->pid,SIGKILL);
    }
    kill(j->pid,SIGKILL);
  }
  return;
}