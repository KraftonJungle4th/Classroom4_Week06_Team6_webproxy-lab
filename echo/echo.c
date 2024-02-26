/*
 * echo - read and echo text lines until client closes connection
 */
/* $begin echo */
#include "csapp.h"

void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;

    Rio_readinitb(&rio, connfd); //rio의 주소에 한 개의 빈 버퍼 설정, 해당 버퍼와 한 개의 오픈한 파일 식별자 연결
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //line:netp:echo:eof //rio의 버퍼에서 한 개의 텍스트 라인을 읽어 buf에 저장
	printf("server received %d bytes\n", (int)n); //서버가 받은 바이트 수 출력
	Rio_writen(connfd, buf, n);//rio의 버퍼에 buf의 내용을 쓰기
    }
}
/* $end echo */
