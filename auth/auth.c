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
#define invInp "500 Syntax error, command unrecognized and the requested action did not take place. This may include errors such as command line too long.\r\n"
#define maxUNameL 50 
#define maxPlen 100
#define retry 1


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

int Send(int connfd, const char* buff, size_t size){
    int val = 0;
    if((val = write(connfd, buff, size))==-1 && sleep(retry) && (val = write(connfd, buff, size)==-1))failureLog("unable to write");
    return val;
}

int Recv(const int connfd, char* buff, const size_t size){
    int val = 0;
    if((val = recv(connfd, buff, sizeof(buff), 0))==-1)failureLog("unable to receive");
    return val;
}


int authHelper(const int connfd, const char* uName){
    char getLine[maxUNameL+maxPlen];
    char getPass[maxPlen];
    FILE* fp = NULL;
    char* p = NULL;
    int found = 0;

    if((fp=fopen("/home/basu/Documents/FTP-server-in-C---RFC-959/auth/users.txt", "r"))==NULL){
        perror("unable to open file\n");
        return -1;
    }
    
    while(fgets(getLine, sizeof(getLine), fp)){
        char temp[maxUNameL];
        sscanf(getLine, "%s %s", temp, getPass);
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
    if((recv(connfd, pass, maxPlen, 0))<0)failureLog("Unable to read passwword\n");
    

    p = strtok(pass, " ");
    if(strcmp(trim(p), "PASS")!=0){
        Send(connfd, invInp, strlen(invInp));
        return -1;
    }

    p = strtok(NULL, "\n");
    
    if(strcmp(trim(p),trim(getPass))==0)return 1;
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
    if((read = recv(connfd, buff, maxUNameL, 0))==-1)failureLog("unable to read");

    inp[0] = trim(strtok(buff, " "));
    inp[1] = trim(strtok(NULL, "\n"));

    if(strcmp(inp[0], "USER")!=0){
        Send(connfd, okNA, strlen(okNA));
        return -1;
    }

    if(strcmp(inp[1], "anonymous")==0 || strcmp(inp[1], "ANONYMOUS")==0)return 0;
    int f = authHelper(connfd, inp[1]);
    return f;
}