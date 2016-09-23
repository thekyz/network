#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int status;
    int socket_fd;
    struct addrinfo hints;
    struct addrinfo *server_info;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    status = getaddrinfo("127.0.0.1", argv[1], &hints, &server_info);
    if  (status != 0) {
        fprintf(stderr, "[C] getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    socket_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if  (socket_fd < 0) {
        fprintf(stderr, "[C] socket error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    status = connect(socket_fd, server_info->ai_addr, server_info->ai_addrlen);
    if  (status != 0) {
        fprintf(stderr, "[C] connect error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);

    return 0;
}

