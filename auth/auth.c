#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../logs/log.c"

#define okanon "332 No need account for login."
#define okUser "331 User name okay, password needed."
#define invUName "430 Invalid Username or Password."
#define okNA "202 Command not implemented, superfluous at this site."
#define maxUNameL 50 
#define maxPlen 100
#define retry 1


int Send(int connfd, const char* buff, size_t size){
    int val = 0;
    if((val = write(connfd, buff, size))==-1 && sleep(retry) && (val = write(connfd, buff, size)==-1))failureLog("unable to write");
    return val;
}


//-1 invalid user, 0 anonymous user, 1 regular user, 2 superuser
int userAuth(const int connfd){
    char buff[maxUNameL];
    char pword[maxPlen];
    char* inp[2];
    int user = -1;
    int read = -1;
    if((read = recv(connfd, buff, maxUNameL, 0))>0){
        char *p = strtok(buff, " ");
        inp[0] = p;
        p = strtok(NULL, "\n");
        inp[1] = p;
        free(p);
        if(strcmp(inp[0], "USER")!=0){
            Send(connfd, okNA, strlen(okNA));
            close(connfd);
            return -1;
        }
        char* getName;
        size_t nameS = 50;
        FILE* fp;
        int found = 0;
        if((fp=fopen("users.txt", "r"))==NULL)failureLog("Fatal, unable to read users.txt");
        
        while(getline(&getName, &nameS, fp)!=-1){
            if(strcmp(strtok(getName, " "), inp[1])==0){
                found =1;
                break;
            }
        }
        free(getName);free(fp);
        if(found){
            if(strcmp(inp[1], "anonymous")==0 || strcmp(inp[1], "ANONYMOUS")==0)user = 0;
            Send(connfd, okUser, strlen(okUser));
            
            if((read = recv(connfd, pword, maxPlen, 0))<0)failureLog("Unable to read passwword");
            // PASS *password here*
            char* p = strtok(pword, " ");
            if(strcmp(p, "PASS")!=0){
                Send(connfd, invUName, strlen(invUName));
                close(connfd);
                return -1;
            }
            user = 1;
        }
        else {
            Send(connfd, invUName, strlen(invUName));
            user = -1;
        }
    }
    else {
        Send(connfd, invUName, strlen(invUName));
        user = -1;
    }
    return user;
}