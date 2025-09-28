#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../logs/log.c"
#include <ctype.h>

#define okAnon "332 No need account for login."
#define okUser "331 User name okay, password needed."
#define okPass "230 User logged in, proceed."
#define invUName "430 Invalid Username or Password."
#define okNA "202 Command not implemented, superfluous at this site."
#define invInp "500 Syntax error, command unrecognized and the requested action did not take place. This may include errors such as command line too long."
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


int authHelper(const int connfd, const char* uName){
    char* getName = NULL;
    size_t nameS = maxUNameL;
    FILE* fp;
    char* p = NULL;
    int found = 0;

    if((fp=fopen("users.txt", "r"))==NULL)failureLog("Fatal, unable to read users.txt");
    
    while(getline(&getName, &nameS, fp)!=-1){
        p = getName;
        if(strcmp(strtok(p, " "), uName)==0){
            found = 1;
            break;
        }
    }

    if(found==0){
            Send(connfd, invUName, strlen(invUName));
            free(p);free(getName);free(fp);
            return -1;
        }

    char *pword;
    if((recv(connfd, pword, maxPlen, 0))<0)failureLog("Unable to read passwword");
    

    p = strtok(pword, " ");
    if(strcmp(p, "PASS")!=0){
        Send(connfd, invInp, strlen(invInp));
        close(connfd);
        free(p);free(getName);free(fp);free(pword);
        return -1;
    }

    free(fp);

    p = strtok(NULL, "\n ");
    char *pass = strtok(getName, " ");
    pass = strtok(NULL, "\n");
    
    
    if(strcmp(p,pass)==0){
        Send(connfd, okPass, strlen(okPass));
        free(p);free(getName);free(pass);free(pword);
        return 1;
    }
    else {
        Send(connfd, invUName, strlen(invUName));
        close(connfd);
        free(p);free(getName);free(pass);free(pword);
        return -1;
    }
}


//-1 invalid user, 0 anonymous user, 1 regular user, 2 superuser
int userAuth(const int connfd){
    char buff[maxUNameL];
    char* inp[2];
    int read = -1;
    if((read = recv(connfd, buff, maxUNameL, 0))==-1)failureLog("unable to read");

    inp[0] = strtok(buff, " ");
    inp[1] = strtok(NULL, "\n");

    if(strcmp(inp[0], "USER")!=0){
        Send(connfd, okNA, strlen(okNA));
        close(connfd);
        return -1;
    }

    if(strcmp(trim(inp[1]), "anonymous")==0 || strcmp(trim(inp[1]), "ANONYMOUS")==0){
        Send(connfd, okAnon, strlen(okAnon));
        return 0;
    }
    return authHelper(connfd, inp[1]);
}