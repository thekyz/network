#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    int status;
    struct addrinfo hints;
    struct addrinfo *serverInfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    status = getaddrinfo("127.0.0.1", "6666", &hints, &serverInfo);
    if  (status != 0) {
        fprintf(stderr, "[C] getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    freeaddrinfo(serverInfo);
}

