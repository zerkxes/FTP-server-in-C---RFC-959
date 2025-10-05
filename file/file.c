#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define home "/home/basu"
#define msgBuff 2048
#define error "451 Requested action aborted. Local error in processing.\r\n"

int userType = -1;
char* currPath = home; // Moving up further than /home is not allowed

int cd(const char* path){
    struct stat sb;
    char temp[msgBuff];
    snprintf(temp, msgBuff, "%s/%s", home, path);
    if(stat(temp, &sb)==0 && S_ISDIR(sb.st_mode)){
        memset(currPath, 0, msgBuff);
        strcpy(currPath, temp);
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
    int n = 0;
    int c = 0;
    while(n<msgBuff && (c = fgetc(fp))!=EOF){
        char a = c;
        if(a == ('\n')){
            readBuff[n++] = '\r';
            readBuff[n++] = '\n';
        }
        else readBuff[n++] = a;
    }
    pclose(fp);
    return readBuff;
}
