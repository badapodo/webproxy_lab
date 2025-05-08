#include "csapp.h"

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    // QUERY_STRING 환경변수에서 인자 추출
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p + 1);
        n1 = atoi(strchr(arg1, '=') + 1);
        n2 = atoi(strchr(arg2, '=') + 1);
    }

    // HTML 콘텐츠 생성
    sprintf(content, "Welcome to add.com: ");
    sprintf(content + strlen(content),
            "THE Internet addition portal.\r\n<p>");
    sprintf(content + strlen(content),
            "The answer is: %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
    sprintf(content + strlen(content),
            "Thanks for visiting!\r\n");

    // HTTP 응답 헤더 출력
    printf("Connection: close\r\n");
    printf("Content-length: %lu\r\n", strlen(content));
    printf("Content-type: text/html\r\n\r\n");

    // HTML 본문 출력
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
