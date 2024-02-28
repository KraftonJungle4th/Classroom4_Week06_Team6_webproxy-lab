#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 16777216 
#define MAX_OBJECT_SIZE 8388608 

#define EOF_TYPE 1
#define HOST_TYPE 2
#define OTHER_TYPE 3

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";
static const char *connection_key = "Connection";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *user_agent_key= "User-Agent";
static const char *host_key = "Host";

void doit(int clientfd);
int parse_uri(char *uri,char *hostname,char *path,int *port);
void build_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio);
char strType(char *buf);
int connect_Server(char *hostname,int port,char *http_header);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);


int main(int argc, char **argv)
{
    int listenfd, clientfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    Signal(SIGPIPE, SIG_IGN);


    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }


    listenfd = Open_listenfd(argv[1]); //브라우저로부터의 연결을 받을 준비를 함 (Socket, Bind, Listen)
    while (1) { // 프록시 서버도 늘 연결을 받을 수 있도록 열어두어야 함
        clientlen = sizeof(clientaddr); // 클라이언트 주소의 크기를 저장
        clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //브라우저로부터의 연결을 받음
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, // 클라이언트의 호스트네임과 포트를 가져옴
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(clientfd);                                             //line:netp:tiny:doit
        Close(clientfd);                                            //line:netp:tiny:close
    }
    return 0;
}

void doit(int clientfd) // 클라이언트로부터 요청을 받아 서버로 전달하고, 서버로부터 받은 응답을 클라이언트에 전달
{
    int serverfd; // 서버와의 연결을 위한 파일 디스크립터
    char server_http_header[MAXLINE]; // 서버에 보낼 HTTP 헤더
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];// 버퍼와 메소드, uri, 버전을 저장할 문자열
    char filename[MAXLINE], cgiargs[MAXLINE]; // 파일이름과 cgiargs를 저장할 문자열
    int port; // 포트번호를 저장할 변수
    rio_t rio, server_rio; //rio 버퍼 구조체

    /* Read request line and headers */
    /* 클라이언트의 요청을 프록시에서 읽고 분석 */
    rio_readinitb(&rio, clientfd); //rio의 주소에 한 개의 빈 버퍼 설정, 해당 버퍼와 한 개의 오픈한 파일 식별자 연결
    if (!rio_readlineb(&rio, buf, MAXLINE)) 
        return;
    printf("%s", buf); 
    sscanf(buf, "%s %s %s", method, uri, version); //buf에서 문자열을 읽어 method, uri, version에 저장
    if (strcasecmp(method, "GET")) {                    
        clienterror(clientfd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }                   

    /* 요청 라인을 parsing하여 method, uri, hostname, port, path를 추출 */
    parse_uri(uri, filename, cgiargs, &port);
    /* Server에 전송할 내용을 server_http_header에 저장 -> 요청라인 형식을 GET /urihere/ HTTP/1.0 (method uri version)으로 변경 */
    build_header(server_http_header, filename, cgiargs, port, &rio);
    /* Server에 연결 */
    serverfd = connect_Server(filename, port, server_http_header); 
    
    if (serverfd<0){
        clienterror(clientfd, method, "404", "Not Found",
                    "Proxy couldn't find this file");
      return;
    }
  
    rio_readinitb(&server_rio, serverfd);
    rio_writen(serverfd,server_http_header,strlen(server_http_header));
    size_t s;
    
    while((s=rio_readlineb(&server_rio,buf,MAXLINE))!=0)
    {
        printf("proxy received %lu bytes,then send\n",s);
        rio_writen(clientfd,buf,s);
    }
    Close(serverfd);
}
/* $end doit */

/* build the http header */
void build_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio)
{
    char buf[MAXLINE],request_hdr[MAXLINE],other_hdr[MAXLINE],host_hdr[MAXLINE];
    char type;
    /*request line*/
    sprintf(request_hdr,requestlint_hdr_format,path);
    /*get other request header for client rio and change it */
    while(Rio_readlineb(client_rio,buf,MAXLINE)>0)
    {
        type = strType(buf);
        if (type == EOF_TYPE)
            break;

        if (type == HOST_TYPE)
        {
            strcpy(host_hdr,buf);
            continue;
        }

        if (type == OTHER_TYPE)
            strcat(other_hdr,buf);
    }
    if(strlen(host_hdr)==0)
    {
        sprintf(host_hdr,host_hdr_format,hostname);
    }
    sprintf(http_header,"%s%s%s%s%s%s%s",request_hdr,host_hdr,conn_hdr,prox_hdr,
	    user_agent_hdr,other_hdr,endof_hdr);
}

/* return the type of the string from the client */
char strType(char *buf)
{
    char type;
    if (strcmp(buf,endof_hdr)==0)
        type = EOF_TYPE;
    else if (!strncasecmp(buf,host_key,strlen(host_key)))
        type = HOST_TYPE;
    else if (!strncasecmp(buf,connection_key,strlen(connection_key))
                  &&!strncasecmp(buf,proxy_connection_key,strlen(proxy_connection_key))
                  &&!strncasecmp(buf,user_agent_key,strlen(user_agent_key)))
         {
             type = OTHER_TYPE;
         }
     return type;
} 

/*Connect to the server*/
inline int connect_Server(char *hostname,int port,char *http_header){
    char portStr[100];
    int rc;
    sprintf(portStr,"%d",port);
    rc = Open_clientfd(hostname,portStr);
    if (rc < 0) {
        fprintf(stderr, "Open_clientfd error: %s\n", strerror(errno));
        return 0;
    }
    return rc;
}

/* parse the uri to get the hostname, file path and port */
int parse_uri(char *uri,char *hostname,char *path,int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    if (strstr(uri,"http://"))
    	hostbegin = uri + 7;
    else
	hostbegin = uri + 8;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; 
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	path[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(path, pathbegin);
    }

    char tmp[MAXLINE];
    sprintf(tmp, "/%s", path);
    strcpy(path, tmp);
    return 0;   
}


/*
 *  * clienterror - returns an error message to the client
 *   */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    sprintf(buf, "%sContent-type: text/html\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}
/* $end clienterror */