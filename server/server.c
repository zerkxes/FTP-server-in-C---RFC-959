/*

This only handles the initial control TCP connection to the client.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define bufsize 1024*1024
#define listenq 5


void err_sys(const char* x){
    perror(x);
    //return appropriate ftp error code;
}

//integer variable type determines an ipv4 vs ipv6 connection based on value; 0 for ipv4 and 1 for ipv6
int init(const int type, void* servaddr, const int port){
    int listenfd, connfd;
    int afinet = type?AF_INET6:AF_INET;
    char buf[bufsize];

    if((listenfd = socket(afinet, SOCK_STREAM, 0))==-1)err_sys("socket error");
    memset(&servaddr, 0, sizeof(servaddr));

    if(type){
        ((struct sockaddr_in6*)(servaddr))->sin6_family = afinet;
        ((struct sockaddr_in6*)(servaddr))->sin6_port = htons(port);
        ((struct sockaddr_in6*)(servaddr))->sin6_addr = in6addr_any;
    }
    else {
        ((struct sockaddr_in*)(servaddr))->sin_family = afinet;
        ((struct sockaddr_in*)(servaddr))->sin_port = htons(port);
        ((struct sockaddr_in*)(servaddr))->sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if(bind(listenfd, servaddr, sizeof(servaddr))==-1)err_sys("bind error");
    if(listen(listenfd, listenq)==-1)err_sys("listen error");

    
}