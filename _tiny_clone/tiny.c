/* 간단한, 웹 반복서버. GET 메서드를 이용하며 정적, 동적 내용을 제공 */
#include "csapp.h"

/* 함수원형 선언 */
void doit(int paramFD);
void read_requesthdrs(rio_t *reqPtr);
int parseURI(char *uri, char *fname, char *cgiargs);
void serveStatic();
void serveDynamic();
void get_filetype();
void clientERROR(int paramFD, char *cause, char *errnum, char *shortMSG, char *longMSG);
void sigchld_handler();

int main(int argc, char **argv)
{
    int lstnSockFD, connSockFD;
    char cli_hostName[MAXLINE], cli_portNumber[MAXLINE];
    socklen_t cli_AddrLen;
    // u32타입
    struct sockaddr_storage cli_Addr;
    // 소켓주소저장소(ss) : 어떠한 주소에도 충분한 크기로 작성된 구조체, 그래서
    // 주소가 아닌 저장소라고 불림. common(family..) + padding 등이 포함됨

    /* 커맨드라인 인수 확인 */
    if (argc != 2)
    {
        fprintf(stderr, "<>빼고 다음과 같이 사용하세요 :%s <portNumber>\n", argv[0]);
        exit(1);
    }

    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
    {
        fprintf(stderr, "signal error\n");
        exit(1);
    }

    lstnSockFD = Open_listenfd(argv[1]);
    // open_listenfd의 wrapper함수
    // optval있고,  passive이고, hostname없고, optval로 포트사용여부 확인함
    // 메모리 할당, SOCK_STREAM생성후 addressInfo입력, 예외처리 후
    // clientSocket을 만들고 clientfd를 생성 후, IP와 connect시도
    // 성공시 clientfd 반환

    // 연결 전까지 단순히 반복문 돌림.
    while (1)
    {
        cli_AddrLen = sizeof(cli_Addr);
        connSockFD = Accept(lstnSockFD, (SA *)&cli_Addr, &cli_AddrLen);
        // Accept의 Wrapper함수
        // SA : 일반적인 소켓주소, 패딩이 포함되면 sockaddr_storage
        // FD, 주소, 주소길이를 받아 새로운 소켓을 만들고 그 소켓의 fd를 반환하는 함수
        Getnameinfo((SA *)&cli_Addr, cli_AddrLen, cli_hostName, MAXLINE, cli_portNumber, MAXLINE, 0);
        // getnameinfo의 wrapper함수.
        // 뭐하는지는 잘 모르겠는데... header에 있다는데 안나옴
        printf("호스트:포트 (%s:%s) 에 연결되었습니다.", cli_hostName, cli_portNumber);
        doit(connSockFD);

        Close(connSockFD);
        // close의 wrapper함수
        // FD를 받아 그 파일을 닫는다.
    }
}

void doit(int paramFD)
{
    int isStatic;
    struct stat fstBuff;
    // 파일의 메타데이터에 관한 구조체, fileStatusBuffer
    char buff[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    // tiny로 들어온 데이터의 임시저장소, HTTP요청인자에 관한 정보
    char fileName[MAXLINE], CGIargs[MAXLINE];
    // ?
    rio_t rIOBuffer;
    // tiny에서 쓸 rIO

    /* 요청라인과 헤더를 읽음 */
    Rio_readinitb(&rIOBuffer, paramFD);
    Rio_readlineb(&rIOBuffer, buff, MAXLINE);
    printf("요청 헤더: \n");
    printf("%s", buff);
    sscanf(buff, "%s %s %s", method, uri, version);
    // buff에서 지정한 형식에 따라 문자열을 분석하고 그 데이터를 변수에 할당.

    if (strcasecmp(method, "GET"))
    // customFunc
    {
        clientERROR(paramFD, method, "501", "Not Implemented", "Tiny는 이 메서드 구현 못했음");
        // customFunc
        return;
    }

    /* GET요청으로부터 URI파싱 */
    isStatic = parseURI(uri, fileName, CGIargs);
    if (stat(fileName, &fstBuff) < 0)
    // 파일이름배열(?)과 상태자료형을 받아, 해당 파일상태를 그 자료형에 넣어주는 함수
    {
        clientERROR(paramFD, fileName, "404", "Not Found", "Tiny는 찾지 못했음");
        return;
    }

    /* 정적 컨텐츠 제공 */
    if (isStatic)
    {
        if (!(S_ISREG(fstBuff.st_mode)) || !(S_IRUSR & fstBuff.st_mode))
        // Is status not regular 또는 사용자가 이 파일을 읽을수 없으면
        {
            clientERROR(paramFD, fileName, "403", "Forbidden", "Tiny는 이 정적파일 금지");
            return;
        }
        serveStatic(paramFD, fileName, fstBuff.st_size);
        // customFunc
    }
    /* 동적 컨텐츠 제공 */
    else
    {
        if (!(S_ISREG(fstBuff.st_mode)) || !(S_IXUSR & fstBuff.st_mode))
        {
            clientERROR(paramFD, fileName, "403", "Forbidden", "Tiny는 이 CGI프로그램 금지");
            return;
        }
        serveDynamic(paramFD, fileName, CGIargs);
        // customFunc
    }
}

void clientERROR(int paramFD, char *cause, char *errnum, char *shortMSG, char *longMSG)
{
    char msgBuff[MAXLINE], body[MAXBUF];
    // 🤔 maxline과 maxbuf는 왜 다른걸까?

    /* HTTP응답 Body */
    sprintf(body, "<html><title>Tiny ERROR</title>\r\n\
<body bgcolor="
                  "ffffff"
                  ">\r\n%s: %s\r\n<p>%s: %s\r\n\
<hr><em>Tiny 웹 서버</em>\r\n",
            errnum, shortMSG, longMSG, cause);

    /* HTTP응답을 buff에 잠시 넣어놨다가 Rio를 통해서 전송(Dynamic은 CGI로 출력시 바로전송) */
    sprintf(msgBuff, "HTTP/1.0 %s %s\r\n", errnum, shortMSG);
    Rio_writen(paramFD, msgBuff, strlen(msgBuff));

    sprintf(msgBuff, "Content-type: text/html; charset=UTF-8\r\n");
    Rio_writen(paramFD, msgBuff, strlen(msgBuff));

    sprintf(msgBuff, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(paramFD, msgBuff, strlen(msgBuff));

    Rio_writen(paramFD, body, strlen(body));
    // 헤더 다 쓰고 body 쓰기
}

/* 기본 Tiny는 요청헤더의 정보가 필요없다 */
void read_requesthdrs(rio_t *reqPtr)
{
    char reqBuff[MAXLINE];

    Rio_readlineb(reqPtr, reqBuff, MAXLINE);
    // 문자를 \n을 만나기 전의 MAXLINE 개만큼 읽음. 반환값 rc는 무시

    while (strcmp(reqBuff, "\r\n"))
    // 문자열 \r\n 찾으면 0반환?
    {
        Rio_readlineb(reqPtr, reqBuff, MAXLINE);
        printf("무시 될 request : %s", reqBuff);
    }
    return;
}

int parseURI(char *uri, char *fname, char *cgiargs)
{
    char *slicingPtr;

    if (!strstr(uri, "cgi-bin"))
    // 문자열 중 부분문자열을 찾는 함수
    // 요청에 CGI를 찾는 문자열이 없으면 = 정적 컨텐츠를 요구하면
    {
        strcpy(cgiargs, "");
        // cgiargs주소의 값인 문자열을 지움
        strcpy(fname, ".");
        // fname도 .으로 바꿈
        strcat(fname, uri);

        if (uri[strlen(uri) - 1] == '/')
            strcat(fname, "home.html");
        return 1;
    }
    else
    // 동적 컨텐츠를 요구하면
    {
        slicingPtr = index(uri, '?');
        // 파일과 인자 나눔, uri안에 ?가없으면 NULL반환
        if (slicingPtr)
        {
            strcpy(cgiargs, slicingPtr + 1);
            // 포인터위치 다음꺼부터 카피
            *slicingPtr = '\0';
        }
        else
            strcpy(cgiargs, "");

        strcpy(fname, ".");
        strcat(fname, uri);

        return 0;
    }
}

void serveStatic(int sockFD, char *fname, int fsize)
// 소켓FD를 받아서 연결에 씀
{
    int sourceFD;
    // 자원FD
    char *sourcePtr, ftype[MAXLINE], msgBuff[MAXBUF];
    // 정적컨텐츠를 위한 자원FD, 자원Ptr, 자원file의 타입

    /* 응답헤더를 msgBuff에 써서 연결소켓에 보냄 */
    get_filetype(fname, ftype);
    sprintf(msgBuff, "HTTP/1.0 200 OK \r\nServer: Tiny Web Server\r\n\
Connection: close\r\nContent-length: %d\r\nContent-type: %s\r\n\r\n",
            fsize, ftype);
    Rio_writen(sockFD, msgBuff, strlen(msgBuff));
    printf("응답 헤더:\n");
    printf("%s", msgBuff);

    /* 같은 방법으로 응답body를 보냄 */
    sourceFD = Open(fname, O_RDONLY, 0); //  Open_readonly?
    // sourcePtr = Mmap(0, fsize, PROT_READ, MAP_PRIVATE, sourceFD, 0);
    // // 매핑될 메모리 주소(0쓰면 커널이 알아서)에 fsize만큼, protect_read라는 메모리 보호옵션으로
    // // 매핑옵션(MAP_PRIVATE)를 지정해, 자원FD를 offset 0기준으로 매핑

    /* 원래 코드조각 */
    // Close(sourceFD);
    // Rio_writen(sockFD, sourcePtr, fsize);
    // // rio_writen의 wrapper함수.
    // // sourcePtr와 fsize을 받아 rIO에다가 씀.
    // Munmap(sourcePtr, fsize);

    /* HW 11.9 */
    sourcePtr = Malloc(fsize);
    Rio_readn(sourceFD, sourcePtr, fsize);
    // fsize Byte만큼 읽는 함수
    Close(sourceFD);
    Rio_writen(sockFD, sourcePtr, fsize);
}

/* Derive filetype from filename */
void get_filetype(char *fname, char *ftype)
{
    if (strstr(fname, ".html"))
        strcpy(ftype, "text/html");
    else if (strstr(fname, ".gif"))
        strcpy(ftype, "image/gif");
    else if (strstr(fname, ".png"))
        strcpy(ftype, "image/png");
    else if (strstr(fname, ".jpg"))
        strcpy(ftype, "image/jpeg");
    // HW 11.7
    else if (strstr(fname, ".mp4"))
        strcpy(ftype, "video/mp4");
    else
        strcpy(ftype, "text/plain");
}

void serveDynamic(int sockFD, char *fname, char *cgiargs)
{
    char msgBuff[MAXLINE], *emptyList[] = {NULL};

    /* HTTP응답의 첫부분 반환 */
    sprintf(msgBuff, "HTTP/1.0 200 OK\r\n");
    Rio_writen(sockFD, msgBuff, strlen(msgBuff));
    sprintf(msgBuff, "Server: Tiny Web Server\r\n");
    Rio_writen(sockFD, msgBuff, strlen(msgBuff));

    if (Fork() == 0)
    // 자식 프로세스 생성
    /* Real server would set all CGI vars here 뭔소리야이게*/
    {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(sockFD, STDOUT_FILENO); //  표준출력을 클라이언트로 리다이렉트?
        // Duplicate to : FD를 복제하여 다른 FD에 할당한다.
        Execve(fname, emptyList, environ); // CGI프로그램 실행
    }
    Wait(NULL);
    // 부모는 기다렸다가 자식을 거둠
}

/* CGI자식들 거둬줌 */
void sigchld_handler()
{
    // 단일용 Chap8 p734
    // printf("Caught SIGINT!\n");
    // exit(0);

    int olderrno = errno;

    while (waitpid(-1, NULL, 0) > 0)
        Sio_puts("Handler reaped child\n");

    if (errno != ECHILD)
        Sio_error("waitpid error");

    errno = olderrno;
}

// /* HW 11.8 맞는건지 잘 모르겠다*/
// pid_t processid;

// int n = 3; // 생성할 자식 프로세스의 수.

// for (int i = 0; i < n; i++)
// {
//     if ((processid = fork()) == 0)
//     {
//         printf("자식프로세스 3개(하드코딩) 생성 %d\n", (int)getpid());
//         sleep(1);
//         exit(0);
//     }
// }

// while (1)
// {
//     if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
//     {
//         fprintf(stderr, "signal error\n");
//         exit(1);
//     }
// }
// /* HW 11.8 */