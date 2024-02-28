/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    // host의 TCP 헤더정보
    rio_t rio;

    /* 예외 처리 */
    if (argc != 3) // 인자카운터가 3이 아니면
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);
    // 네트워크 프로그래밍에서 데이터를 안정적으로 읽고 쓰기 위해 설계된 I/O 라이브러리 또는 함수 세트

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd); // line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */