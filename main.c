#include "server/server.c"
#include "auth/auth.c"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


#define msgBuf 1024
#define ok220 "220 Connected to zerkxes FTP server."
#define okPass "230 User logged in, proceed.\r\n"

int main(int argc, char** argv){
    int connfd  =-1;
    FILE* fp = NULL;
    char buff[msgBuf+2];
    char tempMsg[100];
    if(argc!=2)err_sys("No port provided. Usage <executable> <port number>");

    const int listenfd = init(atoi(argv[1]));

    for(;;){
        if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL))==-1)err_sys("accept error\n");
        
        time_t tick = time(NULL);
        char temp[sizeof(ok220)+sizeof(ctime(&tick))];
        sprintf(temp,"%s (%s)\r\n", ok220, strtok(ctime(&tick), "\n"));

        Send(connfd, temp, strlen(temp));

        const int user = userAuth(connfd);
        if(user==-1){
            if((close(connfd))==-1)err_sys("close conn error\n");
        }
        if((fp = fopen("welcome.txt", "r"))==NULL)err_sys("welcome.txt read error\n");
        fread(buff, msgBuf, 1, fp);
        fclose(fp);
        char sendBuff[msgBuf+4];
        snprintf(sendBuff, sizeof(sendBuff),"%s", buff);
        Send(connfd, buff, strlen(buff));
        
        if((close(connfd))==-1)err_sys("close conn error\n");
    }   
    return 0;
}