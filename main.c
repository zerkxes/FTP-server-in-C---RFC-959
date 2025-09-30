#include "server/server.c"
#include "auth/auth.c"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


#define msgBuf 1024
#define commBuff 5
#define CDUP 6383925617
#define CWD 193452899
#define RETR 6384465730
#define STOR 6384517837
#define STOU 6384517840
#define APPE 6383866635
#define RNFR 6384475069
#define RNTO 6384475528
#define ABOR 6383851369
#define DELE 6383962335
#define RMD 193468904
#define MKD 193463393
#define PWD 193467056
#define LIST 6384254433
#define NOOP 6384332705
#define PORT 6384404682
#define QUIT 6384446856

#define ok220 "220 Connected to zerkxes FTP server."
#define okPass "230 User logged in, proceed.\r\n"
#define badComm "503 Bad sequence of commands.\r\n"

unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

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

        for(;;){
            char recvBuff[msgBuf];
            Recv(connfd, recvBuff, msgBuf);
            char comm[commBuff];
            sscanf(recvBuff, "%s", comm);
            trim(comm);

            switch(hash(comm)){
                case CDUP:
                break;
                case CWD:
                break;
                case RETR:
                break;
                case STOR:
                break;
                case STOU:
                break;
                case APPE:
                break;
                case RNFR:
                //fall through
                case RNTO:
                break;
                case ABOR:
                break;
                case DELE:
                break;
                case RMD:
                break;
                case MKD:
                break;
                case PWD:
                break;
                case LIST:
                break;
                case NOOP:
                break;
                case PORT:
                break;
                default:{
                    Send(connfd, badComm, strlen(badComm));
                    break;
                }
            }
            
        }
        
        if((close(connfd))==-1)err_sys("close conn error\n");
    }   
    return 0;
}