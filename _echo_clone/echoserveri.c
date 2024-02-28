#include "csapp.h"

void echo(int fd);

int main(int argc, char **argv)
{
    int lstnSockFD, connSockFD;
    socklen_t cli_AddrLen;
    // u32타입
    struct sockaddr_storage cli_Addr;
    // 소켓주소저장소(ss) : 어떠한 주소에도 충분한 크기로 작성된 구조체, 그래서
    // 주소가 아닌 저장소라고 불림. common(family..) + padding 등이 포함됨
    char cli_hostName[MAXLINE], cli_portNumber[MAXLINE];
    // hostName? hostIP?

    if (argc != 2)
    {
        fprintf(stderr, "<>빼고 다음과 같이 사용하세요 :%s <portNumber>\n", argv[0]);
        exit(0);
    }

    lstnSockFD = Open_listenfd(argv[1]);
    // open_listenfd의 wrapper함수
    // open_client와 비슷하다.
    // optval있고,  passive이고, hostname없고, optval로 포트사용여부 확인함
    // 메모리 할당, SOCK_STREAM생성후 addressInfo입력, 예외처리 후
    // clientSocket을 만들고 clientfd를 생성 후, IP와 connect시도
    // 성공시 clientfd 반환

    // 연결 전까지 단순히 반복문 돌림. 클라이언트가 2개이상 연결이 안되며, 이같은 서버를
    // iterative server, 반복서버 라고 부른다.
    while (1)
    {
        cli_AddrLen = sizeof(struct sockaddr_storage);
        connSockFD = Accept(lstnSockFD, (SA *)&cli_Addr, &cli_AddrLen);
        // Accept의 Wrapper함수
        // SA : 일반적인 소켓주소, 패딩이 포함되면 sockaddr_storage
        // FD, 주소, 주소길이를 받아 새로운 소켓을 만들고 그 소켓의 fd를 반환하는 함수
        Getnameinfo((SA *)&cli_Addr, cli_AddrLen, cli_hostName, MAXLINE, cli_portNumber, MAXLINE, 0);
        // getnameinfo의 wrapper함수.
        // 뭐하는지는 잘 모르겠는데... header에 있다는데 안나옴
        printf("호스트:포트 (%s:%s) 에 연결되었습니다.", cli_hostName, cli_portNumber);
        echo(connSockFD);
        Close(connSockFD);
        // close의 wrapper함수
        // FD를 받아 그 파일을 닫는다.
    }

    exit(0);
}