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
void read_requesthdrs(rio_t *rp, int fd, int text_file_fd); //  헤더 이후의 나머지를 읽는함수
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd;                  //  듣기소켓, 연결소켓
    char hostname[MAXLINE], port[MAXLINE]; // 호스트명 : 네트워크에 연결된 고유한 장치이름
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; // 클라이언트주소 저장변수

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);  // line:netp:tiny:doit
        Close(connfd); // line:netp:tiny:close
    }
}

void doit(int sockfd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    int text_file_fd;                                                      //  데이터를 저장할 textfile의 fd
    text_file_fd = Open("./data/requestInfo.txt", O_RDWR | O_CREAT, 0644); //  open의 Wrapper함수 Open (csapp.c), mode가 뭔지 잘 모르겠음

    /* Read request line and headers */
    Rio_readinitb(&rio, sockfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    Write(text_file_fd, buf, strlen(buf)); //  buf에 담긴 데이터 만큼 기록
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET"))
    {
        clienterror(sockfd, method, "501", "Not Implemented", "Tiny doesn't implement this method");
        return;
    }
    read_requesthdrs(&rio, sockfd, text_file_fd);
    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0)
    {
        clienterror(sockfd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (is_static) // Serve static content
    {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            clienterror(sockfd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        serve_static(sockfd, filename, sbuf.st_size);
    }
    else // Serve dynamic content
    {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        {
            clienterror(sockfd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(sockfd, filename, cgiargs);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s : %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp, int fd, int text_file_fd)
{
    char *srcp, buf[MAXLINE], header[MAXBUF]; //  🤔헤더의 최대 사이즈는?
    // source pointer
    struct stat sbuf; //  stat buffer

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
        Write(text_file_fd, buf, strlen(buf)); //  파일에 기록
    }

    if (stat("./data/requestInfo.txt", &sbuf) < 0)
    {
        clienterror(fd, "./data/requestInfo.txt", "404", "Not Found", "Tiny couldn't find this file.");
        return;
    }

    sprintf(header, "HTTP/1.1 200 OK\r\n\
Server: Tiny Web Server\r\n\
Connection: close\r\n\
Content-length: %ld\r\n\
Content-type: %s\r\n\r\n",
            sbuf.st_size, "text/plain");
    printf("%s", header);
    // sprintf(header, "HTTP/1.0 200 OK\r\n");
    // sprintf(header, "%sServer1: Tiny Web Server\r\n", header);
    // sprintf(header, "%sConnection2: close\r\n", header);
    // sprintf(header, "%sContent-length3: %ld\r\n", header, sbuf.st_size);
    // sprintf(header, "%sContent-type4: %s\r\n\r\n", header, "text/plain");
    // printf("%s", header);
    Rio_writen(fd, header, strlen(header));
    srcp = Mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, text_file_fd, 0);
    Close(text_file_fd);
    Rio_writen(fd, srcp, sbuf.st_size);
    Munmap(srcp, sbuf.st_size);
    return;
}

void parse_textfile()
{
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin"))
    {
        /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else
    {
        /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, "");

        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);

    sprintf(buf, "HTTP/1.1 200 OK\r\n\
Server: Tiny Web Server\r\n\
Connection: close\r\n\
Content-length: %d\r\n\
Content-type: %s\r\n\r\n",
            filesize, filetype);

    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);

    /* Send respose body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

/*
 * get_filetype - Derive file type from filename
 */

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) /* Child */
    {
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);              // REdirect stdout to client
        Execve(filename, emptylist, environ); // Run CGI program
    }
    Wait(NULL); // Parent waits for and reaps child
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}