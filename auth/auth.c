#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../logs/log.c"
#include <ctype.h>

#define okAnon "332 No need account for login.\r\n"
#define okUser "331 User name okay, password needed.\r\n"
#define invUName "430 Invalid Username or Password.\r\n"
#define okNA "202 Command not implemented, superfluous at this site.\r\n"
#define invInp "500 not understood/\r\n"
#define maxUNameL 50 
#define maxPlen 100
#define retry 1

int gotoCounter = 0;

char *ltrim(char *s)
{
    while(!isalpha(*s)) s++;
    return s;
}

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(!isalpha(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s)); 
}

char *ltrim_s(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

char *rtrim_s(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim_s(char *s)
{
    return rtrim_s(ltrim_s(s)); 
}

int Send(int connfd, const char* buff, size_t size){
    int val = 0;
    if((val = write(connfd, buff, size))==-1
     && sleep(retry)
     && (val = write(connfd, buff, size)==-1))failureLog("unable to write");
    return val;
}

int Recv(const int connfd, char* buff, const size_t size){
    int val = 0;
    if((val = recv(connfd, buff, size, 0))==-1)return -1;
    return val;
}


int authHelper(const int connfd, const char* uName){
    char getLine[maxUNameL+maxPlen];
    char getPass[maxPlen];
    int uType = -1;
    FILE* fp = NULL;
    char* p = NULL;
    int found = 0;

    if((fp=fopen("auth/users.txt", "r"))==NULL){
        perror("unable to open file\n");
        return -1;
    }
    
    while(fgets(getLine, sizeof(getLine), fp)){
        char temp[maxUNameL];
        sscanf(getLine, "%s %s %d", temp, getPass, &uType);
        if(strcmp(trim(temp), uName)==0){
            found = 1;
            break;
        }
    }

    if(found==0){
            Send(connfd, invUName, strlen(invUName));
            return -1;
        }
    
    Send(connfd, okUser, strlen(okUser));

    char pass[maxPlen];
    if((recv(connfd, pass, maxPlen, 0))<0)failureLog("Unable to read password\n");
    

    p = strtok(pass, " ");
    if(strcmp(trim(p), "PASS")!=0){
        Send(connfd, invInp, strlen(invInp));
        return -1;
    }

    p = strtok(NULL, "\n");
    
    if(strcmp(trim(p),trim(getPass))==0){
        return uType;
    }
    else {
        Send(connfd, invUName, strlen(invUName));
        return -1;
    }
}


//-1 invalid user, 0 anonymous user, 1 regular user, 2 superuser
int userAuth(const int connfd){
    char buff[maxUNameL];
    char* inp[2];
    int read = -1;
    i:if(gotoCounter>2){
        gotoCounter = 0;
        return -1;
    }
    if((read = Recv(connfd, buff, maxUNameL))==-1)perror("unable to read");
    inp[0] = malloc(5);
    inp[1] = malloc(maxUNameL);
    sscanf(buff, "%s %s", inp[0], inp[1]);

    if(strcmp(inp[0], "USER")!=0){
        char sendBuff[100];
        snprintf(sendBuff, sizeof(sendBuff), "500 %s command not understood.\r\n", inp[0]);
        Send(connfd, sendBuff, strlen(sendBuff));
        gotoCounter++;
        free(inp[0]);
        free(inp[1]);
        goto i;
        return -1;
    }

    if(strcmp(inp[1], "anonymous")==0 || strcmp(inp[1], "ANONYMOUS")==0){
        free(inp[0]);
        free(inp[1]);
        return 0;
    }
    int f = authHelper(connfd, inp[1]);
    free(inp[0]);
    free(inp[1]);
    return f;
}