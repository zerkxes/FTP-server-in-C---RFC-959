#include "server/server.c"
#include "auth/auth.c"
#include "file/file.c"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define commBuff 5
#define argBuff 1024
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
#define SYST 6384523416
#define PASV 6384389471
#define FEAT 6384033861
#define TYPE 6384559239
#define SIZE 6384506208

#define ok220 "220 Connected to zerkxes FTP server."
#define okPass "230 User logged in, proceed.\r\n"
#define badComm "503 Bad sequence of commands.\r\n"
#define okPasv "227 Entering Passive Mode"
#define okFile "150 Opening ASCII mode data connection for file list"
#define okCloseDataCon "226 Closing data connection. Requested file action successful.\r\n"
#define okAction "250 Requested file action was okay, completed.\r\n"
#define invDir "550 Requested action not taken. No such file or directory.\r\n"
#define okEnv "215 Linux 6.8.0-60-generic #63-Ubuntu GNU/Linux\r\n"
#define okClose "221 Tatah.\r\n"
#define invSyn "501 Syntax error in parameters or arguments.\r\n"

unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void setUserVariables(const int user){
    extern int userType;
    userType = user;
    extern char* currPath;
    currPath = malloc(msgBuff);
    currPath = strcpy(currPath, home);
}

int initDataCon(const int connfd){
    struct tuple in;
    in = dataConInit(lFPort);
    const int listenfd = in.listen;
    const int dataPort = in.port;
    
    int out[6];
    out[0] = 127;
    out[1] = 0;
    out[2] = 0;
    out[3] = 1;
    out[4] = dataPort/256;
    out[5] = dataPort%256;
    int datafd;
    char buff[msgBuff];
    snprintf(buff, sizeof(buff),
     "%s (%d,%d,%d,%d,%d,%d)\r\n",
     okPasv, out[0], out[1], out[2], out[3], out[4], out[5]);
    
    Send(connfd, buff, strlen(buff));

    struct sockaddr_in clientaddr;
    socklen_t clientaddr_size = sizeof(clientaddr);
    if((datafd = accept(listenfd,
         (struct sockaddr*)&clientaddr,
         &clientaddr_size))==-1)err_sys("datacon accept error\r\n");
    return datafd;
}

void closeDataCon(const int datafd){
    shutdown(datafd, SHUT_WR);
    usleep(150000);
    close(datafd);
}

int main(int argc, char** argv){
    int connfd  =-1;
    int datafd = -1;
    FILE* fp = NULL;
    char buff[msgBuff+2];
    char tempMsg[100];
    if(argc!=2){
        err_sys("No port provided. Usage <executable> <port number>");
        exit(1);
    }
    const int listenfd = init(atoi(argv[1]));

    i:for(;;){
        if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL))==-1)err_sys("accept error\n");
        
        time_t tick ;
        tick = time(NULL);
        char temp[sizeof(ok220)+sizeof(ctime(&tick))];//sizeof ctime fix later
        sprintf(temp,"%s (%s)\r\n", ok220, strtok(ctime(&tick), "\n"));

        Send(connfd, temp, strlen(temp));
        
        const int user = userAuth(connfd);
        if(user==-1){
            Send(connfd, "530 Not logged in.\r\n", strlen("530 Not logged in.\r\n"));
            if((close(connfd))==-1)err_sys("close conn error\n");
            goto i;
        }

        if((fp = fopen("welcome.txt", "r"))==NULL)err_sys("welcome.txt read error\n");
        fread(buff, msgBuff, 1, fp);
        fclose(fp);
        char sendBuff[msgBuff+4];
        snprintf(sendBuff, sizeof(sendBuff),"%s\r\n", buff);
        Send(connfd, sendBuff, strlen(sendBuff));
        
        setUserVariables(user);
        char recvBuff[msgBuff];char comm[commBuff];
        char args[argBuff];
        char sendMsgBuff[msgBuff +4];
        char* sendMsgLiteral = NULL;
        int status = -1;
        char type = 'I';

        //start of user session
        for(;;){
            if(Recv(connfd, recvBuff, msgBuff) == -1)break;
            sscanf(recvBuff, "%s %s\r\n", comm, args);
            trim(comm);

            fprintf(stderr,"%s\n%s\n", comm, args);

            switch(hash(comm)){
                case SYST:
                    Send(connfd, okEnv, strlen(okEnv));
                    break;
                case FEAT:
                    sendMsgLiteral = "211-Supported Extensions:\r\n"" PASV\r\n"" PWD\r\n"" LIST\r\n"" CWD\r\n"" CDUP\r\n"" QUIT\r\n"" SIZE\r\n""211 End.\r\n";
                    Send(connfd, sendMsgLiteral, strlen(sendMsgLiteral));
                    break;
                case CDUP:
                    status = cdup();
                    if(status)Send(connfd, okAction, strlen(okAction));
                    break;
                case CWD:
                    sendMsgLiteral = trim(args);
                    status = cd (sendMsgLiteral);
                    if(status == -1) Send(connfd, invDir, strlen(invDir));
                    else {
                        snprintf(sendMsgBuff,
                             msgBuff, "%s %s\r\n",
                             "250 Current Directory changed to :", currPath);
                        Send(connfd, sendMsgBuff, strlen(sendMsgBuff));
                    }
                    break;
                break;
                case TYPE:
                    if(strcmp(trim(args), "A")==0){
                        type = 'A';
                        Send(connfd, "200 Type set to A\r\n", strlen("200 Type set to A\r\n"));
                    }
                    else if(strcmp(trim(args), "I")==0){
                        type = 'I';
                        Send(connfd, "200 Type set to I\r\n", strlen("200 Type set to I\r\n"));
                    }
                    else Send(connfd, invSyn, strlen(invSyn));
                break;
                case SIZE:
                    sendMsgLiteral = trim(args);
                    status = size(sendMsgLiteral);
                    if(status == -1){
                        snprintf(sendMsgBuff, msgBuff, "550 %s: No such file or directory.\r\n", args);
                        Send(connfd, sendMsgBuff, strlen(sendMsgBuff));
                    }
                    else {
                        snprintf(sendMsgBuff, msgBuff, "213 %d \r\n", status);
                        Send(connfd, sendMsgBuff, strlen(sendMsgBuff));
                    }
                    break;
                case RETR:
                    sendMsgLiteral = trim(args);
                    FILE* fp = getFile(sendMsgLiteral);
                    if(fp==NULL){
                        snprintf(sendMsgBuff, msgBuff, "550 %s: No such file or directory.\r\n", args);
                        Send(connfd, sendMsgBuff, strlen(sendMsgBuff));
                        fclose(fp);
                        closeDataCon(datafd);
                        break;//check later
                    }
                    fclose(fp);
                    snprintf(sendMsgBuff,
                         msgBuff, "150 Opening BINARY mode data connection for %s (%d)\r\n",
                         args, size(args));
                    Send(connfd, sendMsgBuff, strlen(sendMsgBuff));

                    status = retrF(datafd, sendMsgLiteral, 'I');
                    if(status == -1)Send(connfd,
                         "451 Requested action aborted. Local error in processing.\r\n",
                         strlen("451 Requested action aborted. Local error in processing.\r\n"));
                    else Send(connfd,
                         "226 Data transfer complete.\r\n",
                         strlen("226 Data transfer complete.\r\n"));
                    closeDataCon(datafd);
                    break;
                case STOR:
                    sendMsgLiteral = trim(args);
                    Send(connfd,
                         "150 Server ready to receive data.\r\n",
                         strlen("150 Server ready to receive data.\r\n"));
                    
                    status = storF(datafd, sendMsgLiteral);
                    if(status == -1)Send(connfd,
                         "451 Requested action aborted. Local error in processing.\r\n",
                         strlen("451 Requested action aborted. Local error in processing.\r\n"));
                    else Send(connfd,
                         "226 Data transfer complete.\r\n",
                         strlen("226 Data transfer complete.\r\n"));
                    closeDataCon(datafd);
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
                    sendMsgLiteral = pwd();
                    snprintf(sendMsgBuff,
                         sizeof(sendMsgBuff), "257 \"%s\" %s\r\n",
                         sendMsgLiteral, "is current working directory");
                    Send(connfd, sendMsgBuff, strlen(sendMsgBuff));
                    break;
                case LIST:
                    sendMsgLiteral = ls();
                    snprintf(sendMsgBuff, msgBuff, "%s\r\n", okFile);
                    Send(connfd, sendMsgBuff, strlen(sendMsgBuff));
                    Send(datafd, sendMsgLiteral, strlen(sendMsgLiteral));
                    free(sendMsgLiteral);
                    Send(connfd, okCloseDataCon, strlen(okCloseDataCon));
                    closeDataCon(datafd);
                    break;
                case NOOP:
                break;
                case PORT:
                break;
                case PASV:
                    close(datafd);
                    datafd = initDataCon(connfd);
                    break;
                case QUIT:
                    Send(connfd, okClose, strlen(okClose));
                    close(connfd);
                    goto u;
                    break;
                default:{
                    Send(connfd, badComm, strlen(badComm));
                    break;
                }
            }
            memset(comm, 0, commBuff);
            memset(recvBuff, 0, msgBuff);
            memset(sendMsgBuff, 0, msgBuff);
            memset(args, 0, argBuff);
            sendMsgLiteral = NULL;
        }
        u:free(currPath);
    }   
    return 0;
}