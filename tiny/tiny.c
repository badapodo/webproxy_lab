#include "csapp.h"

void serve_ssr_adder(int fd, char *cgiargs) {
    char buf[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    if (sscanf(cgiargs, "n1=%d&n2=%d", &n1, &n2) != 2) {
        n1 = n2 = 0;
    }

    sprintf(content,
        "<html><body>"
        "<h1>SSR Adder Result</h1>"
        "<p>The answer is: %d + %d = <strong>%d</strong></p>"
        "<p><a href=\"/adder.html\">Go Back</a></p>"
        "</body></html>", n1, n2, n1 + n2);

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n\r\n", strlen(content));
    Rio_writen(fd, buf, strlen(buf));

    Rio_writen(fd, content, strlen(content));
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    if (strstr(uri, "/ssr_adder")) {
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        } else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, "./ssr_adder"); // dummy filename
        return 2; // SSR
    }
    if (!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");
        sprintf(filename, ".%s", uri);
        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "home.html");
        return 1; // static
    }
    ptr = index(uri, '?');
    if (ptr) {
        strcpy(cgiargs, ptr + 1);
        *ptr = '\0';
    } else {
        strcpy(cgiargs, "");
    }
    sprintf(filename, ".%s", uri);
    return 0; // CGI
}

void serve_static(int fd, char *filename, int filesize, char *method) {
    char filetype[MAXLINE], buf[MAXBUF];
    int srcfd;

    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf + strlen(buf), "Server: Tiny\r\n");
    sprintf(buf + strlen(buf), "Content-length: %d\r\n", filesize);
    sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));

    if (!strcasecmp(method, "HEAD")) return;

    srcfd = Open(filename, O_RDONLY, 0);
    char *srcbuf = malloc(filesize);
    Rio_readn(srcfd, srcbuf, filesize);
    Close(srcfd);
    Rio_writen(fd, srcbuf, filesize);
    free(srcbuf);
}

void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".js"))
        strcpy(filetype, "application/javascript");
    else
        strcpy(filetype, "text/plain");
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body + strlen(body), "<body><p>%s: %s</p><p>%s: %s</p></body></html>\r\n",
            errnum, shortmsg, longmsg, cause);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n\r\n", strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    struct stat sbuf;
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    while (strcmp(buf, "\r\n"))
        Rio_readlineb(&rio, buf, MAXLINE);

    int type = parse_uri(uri, filename, cgiargs);

    if (type == 2) {
        serve_ssr_adder(fd, cgiargs);
        return;
    }

    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
        return;
    }

    serve_static(fd, filename, sbuf.st_size, method);
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        doit(connfd);
        Close(connfd);
    }
}
