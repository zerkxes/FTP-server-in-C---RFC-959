#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define home "/home/basu"
#define msgBuff 1024
#define error "451 Requested action aborted. Local error in processing.\r\n"

int userType = -1;
char* currPath = home; // Moving up further than /home is not allowed

int cd(const char* path){
    struct stat sb;
    if(stat(path, &sb)==0 && S_ISDIR(sb.st_mode)){
        currPath = strcpy(currPath, path);
        return 1;
    }
    else return -1;
}

int cdup(){
    if(strcmp(currPath, home)==0)return 1;
    else {
        int len = strlen(currPath)-1;
        while(currPath[len]!='/')len--;
        currPath[len] = '\0';
        return 1;
    }
}

char* pwd(){
    return currPath;
}

char* ls(){
    char* readBuff = malloc(msgBuff);
    FILE* fp;
    size_t len = strlen("ls -l ") + strlen(currPath);
    char command[msgBuff];
    snprintf(command, sizeof(command), "%s %s", "ls -l ", currPath);

    if((fp = popen(command, "r"))==NULL){
        strcpy(readBuff, error);
        return readBuff;
    }
    fread(readBuff, msgBuff, 1, fp);
    pclose(fp);
    return readBuff;
}
