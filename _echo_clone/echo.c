#include "csapp.h"

/* 커넥트소켓 파일디스크립터 정수값을 받아... rIO에 쓰인 명령어를 받아서 다시 쓰는 함수 */
void echo(int connSockFD)
{
    size_t n;           // 에코로 되돌려 줄 바이트넘버 n
    char buff[MAXLINE]; // echo로 들어온 데이터의 임시저장소??
    rio_t rIOBuffer;    // echo에서 쓸 rIO

    Rio_readinitb(&rIOBuffer, connSockFD);
    // rio_readinitb의 wrapper함수.
    // robust I/O (Rio) package(rIO) 구조체포인터와 연결소켓용FD를 받아,
    // rIO자료구조를 초기화하는 함수.
    // connectSockFD와 robustIOBuffer가 연결된다.
    // rIO구조체에는 rIO전용FD, 카운터, rIO용버퍼포인터, rIO용버퍼가 있다.

    while ((n = Rio_readlineb(&rIOBuffer, buff, MAXLINE)) != 0)
    // 문자를 \n을 만나기 전의 MAXLINE 개만큼 읽음. n은 rc(return code)
    // 즉 n이 정상적인 return code면,
    // 💭 from GPT
    // MAXLINE이 8192개인 경우, 이는 문자의 수가 아니라 바이트 수를 나타냅니다.
    // 따라서 이 값은 읽을 수 있는 한글 문자의 수를 정확히 결정하지 않습니다.
    // 한글은 UTF-8 인코딩을 사용할 때 일반적으로 3바이트로 표현됩니다.
    // 따라서 실제로 읽을 수 있는 한글 문자의 수는 약 8192/3 ≈ 2730글자 정도입니다.
    {
        printf("서버가 %dByte 수신받음 \n", (int)n);
        Rio_writen(connSockFD, buff, n);
        // wrapper함수.
        // FD와 버퍼배열을 받아 rIO에다가 쓸 바이트값N을 받아 rIO에다가 씀.
    }
}