#include "csapp.h"

int main(int argc, char **argv)
{
    int clientFD;
    char *hostIP, *portNumber, buf[MAXLINE];
    rio_t rIOBuffer;

    if (argc != 3)
    // 이 프로그램을 실행시키면서 들어오는 인자가 3개가 아니면, (실행명령어 포함)
    {
        fprintf(stderr, "<>빼고 다음과 같이 사용하세요 :%s <hostIP> <portNumber>\n", argv[0]);
        exit(0);
    }

    hostIP = argv[1];
    portNumber = argv[2];

    clientFD = Open_clientfd(hostIP, portNumber);
    // open_clientfd의 wrapper함수
    // 메모리 할당, SOCK_STREAM생성후 addressInfo입력, 예외처리 후
    // clientSocket을 만들고 clientfd를 생성 후, IP와 connect시도
    // 성공시 clientfd 반환
    Rio_readinitb(&rIOBuffer, clientFD);
    // rio_readinitb의 wrapper함수.
    // robust I/O (Rio) package(rIO) 구조체포인터와 연결소켓용FD를 받아,
    // rIO자료구조를 초기화하는 함수.
    // connectSockFD와 robustIOBuffer가 연결된다.
    // rIO구조체에는 rIO전용FD, 카운터, rIO용버퍼포인터, rIO용버퍼가 있다.

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    // fgets + ferror의 wrapper함수
    // 문자포인터, 정수, FILE(배열같은건가? 무지하게 큰 특수한 자료형으로 생각)포인터를 받아,
    // rptr(pointer for reading) 반환. 즉 파일 끝날때까지 읽음
    {
        Rio_writen(clientFD, buf, strlen(buf));
        // rio_writen의 wrapper함수.
        // FD와 버퍼배열을 받아 rIO에다가 쓸 바이트값N을 받아 rIO에다가 씀.
        Rio_readlineb(&rIOBuffer, buf, MAXLINE);
        // 문자를 \n을 만나기 전의 MAXLINE 개만큼 읽음. 반환값 rc는 무시
        Fputs(buf, stdout);
        // fupts의 wrapper함수
        // 문자포인터, 파일포인터를 받아 문자포인터의 문자열을 stdout에 쓰는 함수.
    }

    Close(clientFD);
    // close의 wrapper함수
    // FD를 받아 그 파일을 닫는다.
    exit(0);
}