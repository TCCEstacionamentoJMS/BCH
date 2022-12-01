#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

//Version of the logger
#define CL_VERSION "1.0.0"

//System-specific constants
#define CL_MAX_USERNAME 31

#ifdef _WIN32
    #define CL_STR_NEWLINE "\r\n"
#else
    #define CL_STR_NEWLINE "\n"
#endif

//General constants
#define CL_MAX_SAMELOGS 128
#define CL_MAX_LOGLEN 512

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

typedef struct logheader {
    time_t date;
    char username[CL_MAX_USERNAME]; 
    pid_t pid;
    char loggerversion[32];
    unsigned char logpriority;
} LOGHDR;

typedef struct logmessage {
    time_t date;
    unsigned char msgpriority;
    unsigned char channel;
    char message[CL_MAX_LOGLEN];
} LOGMSG;


//WRITING FUNCTIONS

// Return the file pointer to a valid log file.
LOG* log_createlog(const char* name, unsigned char priority) {
    LOG* thislog = (LOG *) malloc(sizeof(LOG));
    if(thislog == NULL) return NULL;

    /*If a file of the same name already exists, we create a new file
    with a number appended, so e.g file, file.1, file.2, etc.*/
    {
        FILE* ifn = fopen(name, "r");
        if(ifn != NULL) {
            for(int i = 1; i <= CL_MAX_SAMELOGS; i++) {
                char newname[PATH_MAX];
                snprintf(newname, PATH_MAX, "%s.%d", name, i);

                FILE *tempfn = fopen(newname, "r");
                if(tempfn == NULL) {
                    thislog->logptr = fopen(newname, "w+");
                    if(thislog->logptr == NULL) return NULL;
                    break;
                } else {
                    fclose(tempfn);
                }
            }
            fclose(ifn);
        } else {
            thislog->logptr = fopen(name, "w+");
            if(thislog->logptr == NULL) return NULL;
        }
    }
    //Log information to write
    LOGHDR thislogheader;
    thislogheader.date = time(NULL);
    getlogin_r(thislogheader.username, CL_MAX_USERNAME);
    thislogheader.logpriority = priority;
    strcpy(thislogheader.loggerversion, CL_VERSION);
    thislogheader.pid = getpid();

    //Write the log
    fwrite(&thislogheader, sizeof(LOGHDR), 1, thislog->logptr);

    //Set the number of messages to 0
    thislog->curmsg = 0;

    return thislog;
}

void log_closelog(LOG* log) {
    if(log->logptr != NULL) fclose(log->logptr);
    free(log);
}

//Sends a message, returns the number of the last message sent.
msg_t log_sendmessage(LOG* log, const char* message, unsigned char priority) {
    LOGMSG thislogmessage;
    thislogmessage.msgpriority = priority;
    thislogmessage.date = time(NULL);

    //If the length of the message is above 512 bytes, split it up and repeat with the rest.
    if(strlen(message) > (CL_MAX_LOGLEN - 1)) {
        memcpy(thislogmessage.message, message, CL_MAX_LOGLEN - 1);
        fwrite(&thislogmessage, sizeof(thislogmessage), 1, log->logptr);
        log->curmsg += 1;

        //Re-send from the limit
        log_sendmessage(log, &message[CL_MAX_LOGLEN], priority);
    } else {
        memcpy(thislogmessage.message, message, CL_MAX_LOGLEN);
    }
    //thislogmessage.message = (char *) malloc(strlen(message) * sizeof(char));

    fwrite(&thislogmessage, sizeof(thislogmessage), 1, log->logptr);
    log->curmsg += 1;

    return log->curmsg - 1;
}

//Return the a string equivalent to the priority
void returnpriority(char* str, unsigned char prioritytoread){
    switch (prioritytoread) {
        case PRIORITY_NOTICE:
            strcpy(str, "AVISO");
            break;
        case PRIORITY_NORMAL:
            strcpy(str, "NORMAL");
            break;
        case PRIORITY_HIGH:
            strcpy(str, "ALTA");
            break;
        case PRIORITY_CATASTROPHIC:
            strcpy(str, "CATASTRÓFICA");
            break;
        default:
            strcpy(str, "N.DEFINIDO");
            break;
    }
}

//READING FUNCTIONS

/* Return the pointer to the beginning of the binary log, go trough it, parse it,
dump it to a human-readable text file, and then seek back to the previous position */
int log_dump(LOG* log, const char* path) {
    fpos_t curpos;
    fgetpos(log->logptr, &curpos);
    fseek(log->logptr, 0, SEEK_SET);
    
    FILE* dumppath = fopen(path, "a");
    if(dumppath == NULL) return 1;

    //Dump header
    {
        LOGHDR dumphdr;
        fread(&dumphdr, sizeof(LOGHDR), 1, log->logptr);
        
        struct tm timedata = *localtime(&dumphdr.date);
        char date[128];

        char priority[13];
        returnpriority(priority, dumphdr.logpriority);

        strftime(date, 128, "%c", &timedata);
        fprintf(dumppath, "Log de prioridade %s, gerado na data %s.%sUsando a versão %s da BCH.%sIdentificação do processo na hora era %d.%s", priority, date, CL_STR_NEWLINE, dumphdr.loggerversion, CL_STR_NEWLINE, dumphdr.pid, CL_STR_NEWLINE);
    }
    
    //Dump messages
    {
        while(1) {
            LOGMSG curmsg;
            fread(&curmsg, sizeof(LOGMSG), 1, log->logptr);

            struct tm timedata = *localtime(&curmsg.date);
            char date[128];

            char priority[13];
            returnpriority(priority, curmsg.msgpriority);

            strftime(date, 128, "%c", &timedata);
            if(feof(log->logptr) == 0) {
                fprintf(dumppath,"[%s] (%s) %s%s", priority, date, curmsg.message, CL_STR_NEWLINE);
            } else {
                break;
            }
        }
    }

    fsetpos(log->logptr, &curpos);
    fclose(dumppath);

    return 0;
}

int log_read(LOG* log, msg_t logindex) {
    fpos_t curpos;
    fgetpos(log->logptr, &curpos);
    fseek(log->logptr, sizeof(LOGHDR), SEEK_SET);
    
}