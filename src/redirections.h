#include "global.h"
#include <sys/stat.h>
#include <fcntl.h>

void pipedExec(char** parsed, char** parsedpipe, int pipe_size);

int isPiped(char ** tabcmd, char** strpiped, int cmd_size, char ** str);

bool redirect(char ** parsed, int size);