#include "csapp.h"

/* 두개의 정수를 빼는 CGI프로그램 */
int main(void)
{
    char *queryIncludingTwoInt, *slicingPtr;
    // queryIncludingTwoInt
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    // 빼야할 정수1, 빼야할 정수2
    // 자식프로세스가 이 main을 실행해서, client에게 직접적으로 보낼 content.
    // OSI 7계층의 데이터는 헤더(header)+내용(content, body)라고 부른다.
    int n1 = 0, n2 = 0;

    /* Extract the two arguments */
    if ((queryIncludingTwoInt = getenv("QUERY_STRING")) != NULL)
    // CGI프로그램의 환경변수를 가져오는 함수. 서버는 fork를 호출해 자식프로세스를 생성,
    // execve를 호출해 CGI프로그램을 자식프로세스에서 실행한다. 자식은 CGI프로그램의
    // 환경변수를 요청인자에 알맞게 설정하고, execve를 실행해 CGI프로그램을 실행한다.
    // 환경변수 중에 "QUERY_STRING"이 있는데, 여기에 uri로 요청한 빼야할 두 정수가 들어있다.
    // 이 값을 추출하여 빼야할 두 정수를 구한다.
    {
        slicingPtr = strchr(queryIncludingTwoInt, '&');
        // 문자열에서 특정 문자를 찾는 함수. 지정된 문자의 포인터를 반환함
        *slicingPtr = '\0';
        // strchr로 반환된 문자&의 주소가 p에 담겨있고,
        // queryIncludingTwoInt로 다시 접근해, &부분을 \0으로 대체한다.
        strcpy(arg1, queryIncludingTwoInt);
        // 매개변수포인터2에서 매개변수포인터1로 값을 복사하는 함수.
        strcpy(arg2, slicingPtr + 1);
        // 아까 저장된 주솟값에 주소크기만큼 더해서 다음꺼 값 복사.
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    /* 응답 body(content는 진짜 쓰고싶은 내용 말하는거 같음) */
    sprintf(content, "QUERY_STRING=%s", queryIncludingTwoInt);
    // 두 인자만 들어간 QUERY_STRING 환경변수가 새로 쓰여짐
    sprintf(content, "subtract.com 에 오신걸 환영합니다:\r\n\
여기는 뺄셈세상 입니다.\r\n<p> 정답은 다음과 같습니다: %d - %d = %d\r\n<p>\
방문해 주셔서 감사합니다!\r\n",
            n1, n2, n1 - n2);

    /* printf라고 HTTP응답을 서버에 단순히 보여주는게 아니고, CGI프로그램이
    실행되는 자식프로세서는 표준출력을 가지게 되는데, 이 printf에서 쓰는 모든
    글자가 표준 출력으로 가게 되고, 이 표준출력이 rIO랑 연결되어서 클라이언트로
    전송되는 것 */
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html; charset=UTF-8\r\n\r\n");
    // MIME 양식에 맞게 정확하게 써야한다.

    printf("%s", content);
    // content의 내용역시, HTTP응답헤더뒤에 따라온다.

    fflush(stdout);
    // 파일포인터의 내용을 비우는?함수

    exit(0);
}