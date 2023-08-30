#include "myshell.h"

#define CUSTOMCMD_SIZE 1

char *errormsg[]={"No error",ROUGE("Impossible to fork process"),ROUGE("Exec failed")};
char *customcmd[CUSTOMCMD_SIZE]= {"myls"};
void (*customfct[CUSTOMCMD_SIZE]) (char * directory, char * parameters) = {&myls};

void init() { // Clearing and initializing the shell
    clear();
    printf("---------- WELCOME TO OUR SHELL PROJECT ----------\n\n\n\n");
    printf("Made by Kévin Avot and Grégoire Delacroix\n\n\n\n");
    sleep(1);
}

void printDirectory() {
    char * buff = getcwd(NULL, 0);
    printf("%s>", buff);
    free(buff);
}

Jobs* allJobs; // jobs
Liste* localVars; // local variables
Job *lastJob; // last job in foreground
char * parameters; // parameters of the command
char * directory; // directory or file the command is aimed at

void loop() {
    printDirectory();
    requiredLine(); // waiting for input
}

void jobHandler(int signum) { // handler for child signal -> Remove from background jobs list
    int wstat;
    pid_t pid = waitpid(-1,&wstat,WNOHANG); // pid of child
    if(!pid) return;
    Job *j = allJobs->job; // search it from the jobs list
    while(j!=NULL) {
        if(pid==j->pid) break;
        j=j->suivant;
    }
    if(j) { // the signal if from a child executed in background
        printf("%s (jobs=[%d], pid=%d)\n",j->cmd,j->jobValue,j->pid);
        unsetJob(j->pid,allJobs); // remove it from current jobs
    }
}

void ctrlCHandler(int signum) { // handler for ctrl-c signal -> Clean all memory
    if(lastJob!=NULL && lastJob->state == RUNNING) { // stop the current action
        kill(lastJob->pid,SIGINT);
        lastJob->state=OVER;
    } else { // free memory
        printf("\nClose this shell ?\n1 for Yes\tAnything else for No\n");
        int answer;
        scanf("%d",&answer);
        if(answer) {
            killJobs(allJobs);
            freeVariables(localVars);
            freeJob(allJobs);
            if(lastJob!=NULL) {
                free(lastJob->cmd);
                free(lastJob);
            }
            if(parameters) free(parameters);
            if(directory) free(directory);
            exit(0);
        }
    }
}

void ctrlZHandler(int signum) { // handler for ctrl-z signal -> Put the current action to stopped job
    if (lastJob!=NULL && lastJob->state == RUNNING) { // stop the current action
        lastJob->state = STOPPED;
        setJob(lastJob->cmd, lastJob->pid, allJobs, STOPPED);
        Job *j = allJobs->job; // search it from the jobs list
        while(j!=NULL) {
            if(lastJob->pid==j->pid) break;
            j=j->suivant;
        }
        kill(j->pid,SIGTSTP);
    }
}

/**
 * @brief Get the Mem Segment object
 *
 * @param key
 * @return int*
 */
int * getMemSegment(key_t key, int *shmid) {
    int *shm;

    // Create the segment.
    if ((*shmid = shmget(key, sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    // printf("shmid = %d\n", *shmid);

    // Now we attach the segment to our data space.
    if ((shm = shmat(*shmid, NULL, 0)) == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    return shm;
} // getMemSegment

int requiredLine() {
    char lgcmd[LGCMD_SIZE],*tabcmd2[100][BUFFER_SIZE],*s,**ps;
    pid_t pid;
    int i,j,status,in,out;
    // help distinguishing command executed from command executed by its childrens
    bool fathercmd = false;
    bool using_custom = false;
    // contain command that will be executed after the command contained in str
    char * str_piped[100];
    // contain the first command in a pipeline
    char * str[100];
    // size of str_piped
    int pipe_size = 0;
    
    int shmid;
    int * shm;

    signal(SIGCHLD,jobHandler);
    signal(SIGINT,ctrlCHandler);
    signal(SIGTSTP,ctrlZHandler);
    shm = getMemSegment(2905, &shmid);

    allJobs = malloc(sizeof(*allJobs)); // initialize the list of jobs
    allJobs->job = NULL;

    lastJob = NULL; // last job executed in foreground

    localVars = malloc(sizeof(*localVars)); // local variables
    localVars->variable = NULL;

    parameters = malloc(100 * sizeof(char));
    directory  = malloc(100 * sizeof(char));

    for(;;){
        in=0;
        out=0;
        if(!fgets(lgcmd,LGCMD_SIZE-1,stdin)) break;
        for(s=lgcmd;isspace(*s);s++); // we skip the spaces at the beginning of the sentence
        for(i=0;*s;i++) { // for all the sentence
            tabcmd2[out][in++]=s; // we affect the actual s that represents our position in the sentence
            while(!isspace(*s) && *s != ';') s++; // we go to the end of the actual word
            if(*s == ';') { // multiple commands
                tabcmd2[out++][in]=NULL; // one command per array
                in=0;
            }
            *s++ = '\0'; // we put the end of word
            while(isspace(*s) || *s == ';') s++; // we skip all the spaces and semicolons for the next word
            if(*s == '\0' && in == 0) out--; // if we went to the next array but no need
        }
        if(i){
            if(in) tabcmd2[out][in]=NULL;
            for(j=0;j<=out;j++) { // one processus per task/command
                int to=0,and=0,or=0,index,bg,inSet=0;
                while(1) {
                    // parameters = ""; broke myls parameters
                    index=0;
                    glob_t globbuf;
                    int p[2];
                    bg=0; // children in background
                    if (pipe(p)==ERR) fatalsyserror(1);;
                    char *tabcmd[BUFFER_SIZE] = { NULL };
                    while(tabcmd2[j][to] != NULL && strcmp("&&",tabcmd2[j][to]) && strcmp("||",tabcmd2[j][to])) { // separate each instruction
                        tabcmd[index++] = tabcmd2[j][to++];
                        if(index>1 && strcmp(tabcmd[index-2],"unset") && isVariable(tabcmd[index-1])) { // check to replace the value with the value of an existing variable
                            char *tempVar = valVariable(tabcmd[index-1], localVars);
                            if(tempVar) strcpy(tabcmd[index-1],tempVar);
                        }
                    }
                    if(tabcmd2[j][to] != NULL && strcmp("&&",tabcmd2[j][to]) == 0) { // in the case of multiple commands linked conditionally, execute each command separately one after the other
                        tabcmd[index] = NULL;
                        and = 1;
                    } else if(tabcmd2[j][to] != NULL && strcmp("||",tabcmd2[j][to]) == 0) {
                        tabcmd[index] = NULL;
                        or = 1;
                    }

                    if (strcmp(*tabcmd2[j], "cd") == 0) {
                        fathercmd = true; // the father executed the cmd
                        if (tabcmd2[j][1] == NULL) mycd("~"); // if no directory is set we cd to home directory
                        else mycd(tabcmd2[j][1]);
                    } else if (strcmp(*tabcmd2[j],"set") == 0 || strcmp(*tabcmd2[j],"setenv") == 0 || strcmp(*tabcmd2[j],"unset") == 0 || strcmp(*tabcmd2[j],"unsetenv") == 0) {
                        inSet = 1; // impossible to set in background
                    } else if (strcmp(*tabcmd2[j], "exit") == 0) {
                        fathercmd = true; // the father executed the cmd
                        killJobs(allJobs);
                        freeVariables(localVars);
                        freeJob(allJobs);
                        if(lastJob!=NULL) {
                            free(lastJob->cmd);
                            free(lastJob);
                        }
                        if(parameters) free(parameters);
                        if(directory) free(directory);
                        myexit(tabcmd2[j][1]);
                    }
                    if(strcmp(tabcmd[index-1],"&")==0) { // put the son in background -> job list
                        if(inSet) printf("Impossible to use set function in background\n");
                        tabcmd[index-1] = NULL;
                        index--;
                        bg=1;
                    }
                    if((pid=fork()) == ERR) fatalsyserror(1);
                    if(!pid && !fathercmd) { // execute the next command except if father already executed it
                        signal(SIGTSTP,SIG_DFL);
                        for (int k = 0; k < CUSTOMCMD_SIZE; k++) {
                            if (strcmp(*tabcmd, customcmd[k]) == 0) {
                                using_custom = true;
                                int c = 0;
                                for (int m = 1; m < index; m++) {
                                    if (!strncmp(tabcmd[m], "-", 1)) {
                                        // parameter
                                        strcat(parameters, tabcmd[m]);
                                    } else {
                                        // not a parameter
                                        strcat(directory, tabcmd[m]);
                                        sprintf(directory, "%s ", directory);
                                        c++;
                                    }
                                }
                                // launch the function associated to the cmd
                                if (c == 0) (*customfct[k])(directory, parameters);
                                else {
                                    char * temp = strtok(directory, " ");
                                    for (int m = 0; m < c; m++) {
                                        printf("%s:\n", temp);
                                        (*customfct[k])(temp, parameters);
                                        printf("\n");
                                        temp = strtok(NULL, " ");
                                    }
                                }
                                free(parameters);
                                free(directory);
                                freeVariables(localVars);
                                freeJob(allJobs);
                                if(lastJob) {
                                    free(lastJob->cmd);
                                    free(lastJob);
                                }
                                exit(0);
                            }
                        }

                        if (!using_custom) {
                            free(directory);
                            free(parameters);
                        }

                        if (redirect(tabcmd, index)) {
                            freeVariables(localVars);
                            freeJob(allJobs);
                            if(lastJob != NULL) {
                                free(lastJob->cmd);
                                free(lastJob);
                            }
                            exit(0);
                        }
                        if ((pipe_size = isPiped(tabcmd2[j], str_piped, index, str)) > 0) {

                            pipedExec(str, str_piped, pipe_size);

                            // end of pipelines
                            pipe_size = 0;
                        }

                        if(strcmp("set",*tabcmd) == 0 || strcmp("setenv",*tabcmd) == 0 || strcmp("unset",*tabcmd) == 0 || strcmp("unsetenv",*tabcmd) == 0) { // set local or environment variable
                            // free(directory);
                            int retour = manageVariables(p,tabcmd,index,localVars); // send value to the father
                            freeVariables(localVars);
                            freeJob(allJobs);
                            if(lastJob != NULL) {
                                free(lastJob->cmd);
                                free(lastJob);
                            }
                            exit(retour);
                        } else if(strcmp("myjobs",*tabcmd) == 0) { // see all jobs
                            // free(directory);
                            getAllJobs(allJobs);
                            freeVariables(localVars);
                            freeJob(allJobs);
                            if(lastJob != NULL) {
                                free(lastJob->cmd);
                                free(lastJob);
                            }
                            exit(0);
                        } else if(strcmp("status",*tabcmd) == 0) { // see last job executed in foreground
                            // free(directory);
                            printJob(lastJob);
                            freeVariables(localVars);
                            freeJob(allJobs);
                            if(lastJob != NULL) {
                                free(lastJob->cmd);
                                free(lastJob);
                            }
                            exit(0);
                        }
                        freeVariables(localVars);
                        freeJob(allJobs);
                        if(lastJob != NULL) {
                            free(lastJob->cmd);
                            free(lastJob);
                        }
                        if(!myglob(globbuf, tabcmd,index)) exit(0); // check for wildcards
                        execvp(*tabcmd,tabcmd);
                        syserror(2);
                        exit(FAILED_EXEC);
                    } else { // wait for his sons to finish their tasks
                        close(p[1]);
                        if(bg && !inSet) { // the child is executed in background, append it to the job list
                            char *cmd = malloc(MAX*sizeof(char));
                            strcpy(cmd,"");
                            for(int n=0;n<index;n++) strcat(cmd,tabcmd[n]);
                            strcat(cmd,"\0");
                            setJob(cmd,pid,allJobs,RUNNING);
                            free(cmd);
                        } else {
                            inSet=0;
                            if(lastJob != NULL) {
                                free(lastJob->cmd);
                                free(lastJob); // replace the last job in foreground
                            }
                            lastJob = malloc(sizeof(*lastJob));
                            char *cmd = malloc(MAX*sizeof(char));
                            strcpy(cmd,"");
                            for(int n=0;n<index;n++) strcat(cmd,tabcmd[n]);
                            strcat(cmd,"\0");
                            lastJob->cmd = cmd;
                            lastJob->pid = pid;
                            lastJob->state = RUNNING;
                            waitpid(pid,&status,WUNTRACED|WCONTINUED); // wait the current child
                            if(WIFEXITED(status)){ // print the commmand status
                                status=WEXITSTATUS(status);
                                lastJob->retour = status;
                                lastJob->state = OVER;
                                if(status != FAILED_EXEC || fathercmd){
                                    if(status == 47) { // the son wants to create local variable
                                        char infos[BUFFER_SIZE];
                                        read(p[0],infos,sizeof(char) * BUFFER_SIZE); // get informations from the pipe
                                        status = setLocalVariable(infos, localVars);
                                    } else if(status == 57) { // the son wants to delete local variable
                                        char infos[BUFFER_SIZE];
                                        read(p[0],infos,sizeof(char) * BUFFER_SIZE); // get informations from the pipe
                                        status = unsetVariable(infos, localVars);
                                    }
                                    printf(VERT("exit status of ["));
                                    for(ps=tabcmd;*ps;ps++) printf("%s",*ps);
                                    printf(VERT("]=%d\033[0m\n"),status);
                                    if(or) break;
                                } else if(and) break;
                            } else {
                                lastJob->retour = status;
                                if(status!=5247) lastJob->state = OVER;
                                else lastJob->state = STOPPED;
                                puts(ROUGE("Anormal exit"));
                            }
                        }
                        close(p[0]);
                        fathercmd = false;
                        using_custom = false;
                    }
                    if(tabcmd2[out][to++] == NULL) break;
                }
            }
        }
        printDirectory();
    }
    /* detach shared memory segment */  
    shmdt(shm);
    /* remove shared memory segment */  
    shmctl(shmid, IPC_RMID, NULL);
    free(parameters);
    free(directory);
    freeVariables(localVars);
    freeJob(allJobs);
    if(lastJob != NULL) {
        free(lastJob->cmd);
        free(lastJob);
    }
    exit(0);
}