#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define PORT "5551"

int main(int argc, char *argv[])
{
    int server_socket;
    int status;
    struct addrinfo hints;
    struct addrinfo *server_info;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;        // Fill own IP automatically

    status = getaddrinfo(NULL, PORT, &hints, &server_info);
    if  (status != 0) {
        fprintf(stderr, "[S] getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (server_socket < 0) {
        fprintf(stderr, "[S] socket error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    status = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (status < 0) {
        fprintf(stderr, "[S] setsockopt error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    status = bind(server_socket, server_info->ai_addr, server_info->ai_addrlen);
    if (status < 0) {
        fprintf(stderr, "[S] bind error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    status = listen(server_socket, 10);
    if (status < 0) {
        fprintf(stderr, "[S] listen error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    printf("[S] Server listening on port %s ...\n", PORT);

    struct sockaddr_storage remote_address;
    socklen_t address_length = sizeof(remote_address);

    int client_socket = accept(server_socket, (struct sockaddr *)&remote_address, &address_length);
    if (client_socket < 0) {
        fprintf(stderr, "[S] accept error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    printf("[S] Client connected !\n");

    char buffer[256];
    int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes == 0) {
        // connection closed
        fprintf(stderr, "[S] Client disconnected.\n");
        freeaddrinfo(server_info);
        return -1;
    } else if (bytes < 0) {
        fprintf(stderr, "[S] recv error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    printf("[S] <<< %s\n", buffer);

    bytes = send(client_socket, "world", 6, 0);
    //TODO: in a real program, you should check wether all bytes were sent !
    if (bytes < 0) {
        fprintf(stderr, "[S] send error: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);
    return 0;
}

