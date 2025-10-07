#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define home getenv("HOME")
#define msgBuff 2048
#define sendFChunk 1048576
#define recvFChunk 10485760
#define error "451 Requested action aborted. Local error in processing.\r\n"

int userType = -1;
char* currPath =NULL; // Moving up further than /home is not allowed

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
    readBuff[n]='\r';
    readBuff[n+1]='\n';
    readBuff[n+2]='\0';
    pclose(fp);
    return readBuff;
}

FILE* getFile(const char* fname){
    FILE* fp;
    int len = strlen(fname) + strlen(pwd());
    char path[len + 2];
    snprintf(path, sizeof(path), "%s/%s", pwd(), fname);
    if((fp = fopen(path, "rb"))==NULL){
        fclose(fp);
        if(ENOENT == errno) fprintf(stderr, "%s", "file does not exist");
        return NULL;
    }
    return fp;//remember to close this
}

FILE* getPath(const char* path){
    FILE* fp;
    if((fp = fopen(path, "r"))==NULL){
        fclose(fp);
        if(ENOENT == errno) fprintf(stderr, "%s", "file does not exist");
        return NULL;
    }
    return fp;//remember to close this
}

int size(const char* fname){
    FILE* fp = getFile(fname);
    if(fp==NULL)return -1;
    unsigned int size;
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);
    return size;
}

int retrF(const int dataCon, const char* fname, const char type){
    FILE* fp = getFile(fname);
    if(fp==NULL)return -1;

    char* freadChunk = malloc(sendFChunk);
    size_t nbytes = 0;
    
    int yes = 1;
    int res = 0;
    if((res = setsockopt(dataCon,
         IPPROTO_TCP,
         TCP_NODELAY,
         &yes,
         sizeof(int)))==-1)fprintf(stderr, "%s", "failed to set tcp_nodelay");

    while((nbytes = fread(freadChunk, 1, sendFChunk, fp))>0){
        int offset = 0;
        int sent = 0;
        while((sent = write(dataCon, freadChunk+offset, nbytes))>0 || (sent==-1 && EINTR==errno)){
            if(sent>0){
                offset+=sent;
                nbytes-=sent;
            }
        }
        memset(freadChunk, 0, sendFChunk);
    }
    fclose(fp);
    free(freadChunk);
    return 1;
}

int storF(const int dataCon, const char* fname){
    FILE* fp;
    int len = strlen(fname) + strlen(pwd());
    char path[len + 2];
    snprintf(path, sizeof(path), "%s/%s", pwd(), fname);
    if((fp = fopen(path, "wb")) == NULL)return -1;
    char* fwrChunk = malloc(recvFChunk);

    int yes = 1;
    int res = 0;
    if((res = setsockopt(dataCon,
         IPPROTO_TCP,
         TCP_NODELAY,
         &yes,
         sizeof(int)))==-1)fprintf(stderr, "%s", "failed to set tcp_nodelay");
    
    size_t nbytes = 0;
    while((nbytes = read(dataCon, fwrChunk, recvFChunk))>0){
        int offset = 0;
        int recv = 0;
        fwrite(fwrChunk, recvFChunk, 1, fp);
        memset(fwrChunk, 0, recvFChunk);
    }
    fclose(fp);
    free(fwrChunk);
    return 1;
}

int mv(const char* oldP, const char* newP){
    char comm[msgBuff];
    snprintf(comm, msgBuff, "mv %s %s", oldP, newP);
    return system(comm);
}