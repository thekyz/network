#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define ADDRESS "127.0.0.1"
#define PORT "5551"

int main(int argc, char *argv[])
{
    int status;
    int socket_fd;
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    status = getaddrinfo(ADDRESS, PORT, &hints, &server_info);
    if  (status != 0) {
        fprintf(stderr, "[C] getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    //TODO: check all server founds !

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

    printf("[C] Connected to server !\n");

    int bytes = send(socket_fd, "hello", 6, 0);
    //TODO: check wether all bytes were sent !
    if (bytes < 0) {
        fprintf(stderr, "[C] send error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    char buffer[256];
    bytes = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes == 0) {
        fprintf(stderr, "[C] Server disconnected. %s\n");
        freeaddrinfo(server_info);
        return -1;
    } else if (bytes < 0) {
        fprintf(stderr, "[C] recv error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    printf("[C] <<< %s\n", buffer);

    close(socket_fd);

    freeaddrinfo(server_info);

    return 0;
}

