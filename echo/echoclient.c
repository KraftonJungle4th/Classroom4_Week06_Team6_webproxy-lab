/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio; // rio 버퍼 구조체

    if (argc != 3) {
	fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	exit(0);
    }
    host = argv[1]; //ip주소
    port = argv[2]; //포트번호

    clientfd = Open_clientfd(host, port); //서버에 연결 
    Rio_readinitb(&rio, clientfd); //rio의 주소에 한 개의 빈 버퍼 설정, 해당 버퍼와 한 개의 오픈한 파일 식별자 연결

    while (Fgets(buf, MAXLINE, stdin) != NULL) { 
	Rio_writen(clientfd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
	Rio_readlineb(&rio, buf, MAXLINE); //rio의 버퍼에서 한 개의 텍스트 라인을 읽어 buf에 저장
	Fputs(buf, stdout); //buf의 내용을 표준출력에 쓰기
    }
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */