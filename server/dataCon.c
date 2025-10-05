#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#define lFPort 32768
#define mFPort 60999

struct tuple {
    int listen;
    int port;
};

void err_sys(const char* x){
    perror(x);
    fprintf(stderr, "%s\n", x);
}

struct tuple dataConInit(int port){
    int listenfd;
    struct tuple out;
    struct sockaddr_in servaddr;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0))==-1)err_sys("socket error\n");
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int num = 0;
    if((num = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) == -1){
        for(int i=port;i<=mFPort;i++){
            return dataConInit(i+1);
        }
        if(port > mFPort){
            err_sys("no ports available");
            return out;
        }
    }
    if(listen(listenfd, 1)==-1)err_sys("listen failure after log");
    out.listen = listenfd;
    out.port = port;
    return out;
}