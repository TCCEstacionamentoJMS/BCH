#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

//Version of the logger
#define CL_VERSION "1.0.0"

//Log priority levels
#define PRIORITY_NORMAL         0
#define PRIORITY_NOTICE         1
#define PRIORITY_HIGH           2
#define PRIORITY_CATASTROPHIC   3

//Datatypes
typedef long int msg_t;

typedef struct logdata {
    FILE* logptr;
    msg_t curmsg;
} LOG;

LOG* log_createlog(const char* name, unsigned char priority);
void log_closelog(LOG* log);
msg_t log_sendmessage(LOG* log, const char* message, unsigned char priority);
int log_dump(LOG* log, const char* path);