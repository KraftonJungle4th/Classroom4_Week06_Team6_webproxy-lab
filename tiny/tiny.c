/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); 
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd){
  int is_static;
  struct stat sbuf; //파일의 상태를 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; 
  char filename[MAXLINE], cgiargs[MAXLINE]; 
  rio_t rio; //rio 버퍼 구조체
  printf("%d",buf);

  /* 헤더로부터 요청 라인 읽기 / Read request line and headers */  
  Rio_readinitb(&rio, fd); //rio의 주소에 한 개의 빈 버퍼 설정, 해당 버퍼와 한 개의 오픈한 파일 식별자 연결
  Rio_readlineb(&rio, buf, MAXLINE); //rio의 버퍼에서 한 개의 텍스트 라인을 읽어 buf에 저장
  printf("Request headers:\n"); 
  printf("%s", buf); 
  sscanf(buf, "%s %s %s", method, uri, version); //buf에서 문자열을 읽어 method, uri, version에 저장
  if (strcasecmp(method, "GET")){ //method가 GET이 아니면
    clienterror(fd, method, "501", "Not Implemented", //에러메시지 출력
                "Tiny does not implement this method"); 
    return;
  }
  read_requesthdrs(&rio); //rio의 헤더를 읽어서 버퍼에 저장

  /* URI 분석 / Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs); //uri를 분석해서 filename과 cgiargs에 저장  
  if(stat(filename, &sbuf) < 0){ //filename의 상태를 sbuf에 저장
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file"); //에러메시지 출력
    return;
  }
  
  if (is_static){  
    if (!S_ISREG(sbuf.st_mode) || (!S_IRUSR & sbuf.st_mode)){ //파일이 보통파일이 아니거나 읽기 권한이 없으면
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file"); //에러메시지 출력
      return;
    }
  serve_static(fd, filename, sbuf.st_size); //정적 컨텐츠 제공
  }
  else{
    if(!S_ISREG(sbuf.st_mode) || (!S_IXUSR & sbuf.st_mode)){ //파일이 보통파일이 아니거나 실행 권한이 없으면
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the GCI program"); //에러메시지 출력
      return;
    }
  serve_dynamic(fd, filename, cgiargs); //동적 컨텐츠 제공
  }  
}

void clienterror(int fd, char*cause, char *errnum, char *shortmsg, char *longmsg){

  char buf[MAXLINE], body[MAXBUF];

  /* HTTP 응답 바디 생성 - Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>"); //body에 문자열 저장
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); //body에 문자열 저장
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg); //body에 문자열 저장
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause); //body에 문자열 저장
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body); //body에 문자열 저장
}