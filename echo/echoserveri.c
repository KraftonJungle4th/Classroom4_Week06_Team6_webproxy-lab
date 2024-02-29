/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

void echo(int connfd);
    
int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE]; //클라이언트의 호스트네임과 포트번호를 저장할 변수

    if (argc != 2) { //명령행 인자가 2개가 아니면
	fprintf(stderr, "usage: %s <port>\n", argv[0]); //에러메시지 출력
	exit(0);
    }

    listenfd = Open_listenfd(argv[1]); //서버를 열고, 서버의 파일 식별자를 반환
    while (1) {
	clientlen = sizeof(struct sockaddr_storage); 
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //클라이언트의 연결을 기다리고, 클라이언트의 파일 식별자를 반환
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
	echo(connfd); //클라이언트와 통신
	Close(connfd); //클라이언트와의 연결 종료
    }
    exit(0);
}
/* $end echoserverimain */