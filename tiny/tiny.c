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

  /* HTTP 응답 헤더 생성 - Build the HTTP response header */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); //buf에 문자열 저장
  Rio_writen(fd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
  sprintf(buf, "Content-type: text/html\r\n"); //buf에 문자열 저장
  Rio_writen(fd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); //buf에 문자열 저장
  Rio_writen(fd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
  Rio_writen(fd, body, strlen(body)); //rio의 버퍼에 body의 내용을 쓰기
}

void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE]; //버퍼 생성
  Rio_readlineb(rp, buf, MAXLINE); //rio의 버퍼에서 한 개의 텍스트 라인을 읽어 buf에 저장
  while(strcmp(buf, "\r\n")){ //buf가 "\r\n"이 아니면
    Rio_readlineb(rp, buf, MAXLINE); //rio의 버퍼에서 한 개의 텍스트 라인을 읽어 buf에 저장
    printf("%s", buf); //buf 출력
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr; //포인터 생성
  if(!strstr(uri, "cgi-bin")){ //uri에 "cgi-bin"이 없으면
    strcpy(cgiargs, ""); //cgiargs에 "" 저장
    strcpy(filename, "."); //filename에 "." 저장
    strcat(filename, uri); //filename에 uri를 이어붙임
    if(uri[strlen(uri)-1] == '/') //uri의 마지막 문자가 '/'이면
      strcat(filename, "home.html"); //filename에 "home.html"을 이어붙임
    return 1; //1 반환
  }
  else{
    ptr = index(uri, '?'); //uri에서 '?'의 위치를 ptr에 저장
    if(ptr){ //ptr이 NULL이 아니면
      strcpy(cgiargs, ptr+1); //cgiargs에 ptr+1의 내용을 복사
      *ptr = '\0'; //ptr에 '\0' 저장
    }
    else
      strcpy(cgiargs, ""); //cgiargs에 "" 저장
    strcpy(filename, "."); //filename에 "." 저장
    strcat(filename, uri); //filename에 uri를 이어붙임
    return 0; //0 반환
  }
}

void serve_static(int fd, char *filename, int filesize){
  int srcfd; //파일 식별자 생성
  char *srcp, filetype[MAXLINE], buf[MAXBUF]; //포인터 생성, 문자열 생성, 버퍼 생성
  get_filetype(filename, filetype); //filename의 파일 타입을 filetype에 저장
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); //buf에 문자열 저장
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); //buf에 문자열 저장
  sprintf(buf, "%sConnection: close\r\n", buf); //buf에 문자열 저장
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize); //buf에 문자열 저장
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); //buf에 문자열 저장
  Rio_writen(fd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
  printf("Response headers:\n"); //출력
  printf("%s", buf); //buf 출력

  srcfd = Open(filename, O_RDONLY, 0); //filename을 읽기 전용으로 오픈
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //srcfd의 파일을 filesize만큼 메모리에 매핑 
  
  srcp = (char *)malloc(filesize); //filesize만큼의 메모리 할당
  Rio_readn(srcfd, srcp, filesize); //srcfd의 내용을 srcp에 저장
  Close(srcfd); //srcfd 닫기
  Rio_writen(fd, srcp, filesize); //rio의 버퍼에 srcp의 내용을 쓰기
  // Munmap(srcp, filesize); //srcp의 메모리 매핑 해제
  free(srcp); //srcp의 메모리 해제
}

void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[] = {NULL}; //버퍼 생성, 포인터 배열 생성
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); //buf에 문자열 저장
  Rio_writen(fd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
  sprintf(buf, "Server: Tiny Web Server\r\n"); //buf에 문자열 저장
  Rio_writen(fd, buf, strlen(buf)); //rio의 버퍼에 buf의 내용을 쓰기
  if(Fork() == 0){ //자식 프로세스 생성
    setenv("QUERY_STRING", cgiargs, 1); //환경변수 설정
    Dup2(fd, STDOUT_FILENO); //fd를 표준 출력으로 복사
    Execve(filename, emptylist, environ); //filename을 실행
  }
  Wait(NULL); //자식 프로세스가 종료될 때까지 기다림
}

void get_filetype(char *filename, char *filetype){
  if(strstr(filename, ".html")) //filename에 ".html"이 있으면
    strcpy(filetype, "text/html"); //filetype에 "text/html" 저장
  else if(strstr(filename, ".gif")) //filename에 ".gif"이 있으면
    strcpy(filetype, "image/gif"); //filetype에 "image/gif" 저장
  else if(strstr(filename, ".jpg")) //filename에 ".jpg"이 있으면
    strcpy(filetype, "image/jpeg"); //filetype에 "image/jpeg" 저장
  else if (strstr(filename, ".MP4")) //filename에 ".mp4"이 있으면
    strcpy(filetype, "video/MP4"); //filetype에 "video/mp4" 저장
  else
    strcpy(filetype, "text/plain"); //filetype에 "text/plain" 저장
}