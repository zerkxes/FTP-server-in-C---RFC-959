#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#define LISTENQ 5

void err_sys(const char* x){
    perror(x);
}

int init(int port){
    int listenfd;
    struct sockaddr_in servaddr;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0))==-1)err_sys("socket error\n");
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))==-1)err_sys("bind error\n");
    if(listen(listenfd, LISTENQ)==-1)err_sys("listen error\n");
    return listenfd;
}