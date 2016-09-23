#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define MAX_CONNECTIONS 16

#define MSG_IN          "IN"
#define MSG_OUT         "OUT"
#define MSG_OK          "OK"

static int start_server(char *server_port)
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

    status = getaddrinfo(NULL, server_port, &hints, &server_info);
    if  (status != 0) {
        fprintf(stderr, "--- getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (server_socket < 0) {
        fprintf(stderr, "--- socket error: %s\n", strerror(errno));
        return -2;
    }

    status = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (status < 0) {
        fprintf(stderr, "--- setsockopt error: %s\n", strerror(errno));
        return -3;
    }

    status = bind(server_socket, server_info->ai_addr, server_info->ai_addrlen);
    if (status < 0) {
        fprintf(stderr, "--- bind error: %s\n", strerror(errno));
        return -4;
    }

    freeaddrinfo(server_info);
    return server_socket;
}

int server_loop(int server_socket, char *server_port)
{
    fd_set read_set, master_set;
    int rc = 0;
    int it, broadcast_it;
    int selected = 0;
    int select_max;
    char buffer[256];

    FD_ZERO(&read_set);
    FD_ZERO(&master_set);

    rc = listen(server_socket, MAX_CONNECTIONS);
    if (rc < 0) {
        rc = errno;
        fprintf(stderr, "--- listen error: %s\n", strerror(rc));
        return -1;
    }

    printf("+++ Listening on port '%s' ...\n", server_port);

    FD_SET(server_socket, &master_set);
    select_max = server_socket;

    for (;;) {
        memset(buffer, 0, sizeof(buffer));
        read_set = master_set;

        selected = select(select_max + 1, &read_set, NULL, NULL, NULL);
        if (selected < 0) {
            rc = errno;
            fprintf(stderr, "--- select error: %s\n", strerror(errno));
            return rc;
        }

        for (it = 0; it <= select_max; it++) {
            if (!FD_ISSET(it, &read_set)) {
                continue;
            }

            if (it == server_socket) {
                struct sockaddr_storage remote_address;
                socklen_t address_length = sizeof(remote_address);
                int client_socket = accept(server_socket, (struct sockaddr *)&remote_address, &address_length);
                if (client_socket < 0) {
                    fprintf(stderr, "--- accept error: %s\n", strerror(errno));
                    continue;
                }

                FD_SET(client_socket, &master_set);

                if (client_socket > select_max) {
                    select_max = client_socket;
                }

                printf(">>> Client %d connected.\n", client_socket);
            } else {
                int bytes = recv(it, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    if (bytes == 0) {
                        // connection closed
                        printf("<<< Client %d disconnected.\n", it);
                    } else {
                        fprintf(stderr, "--- recv error: %s\n", strerror(errno));
                    }

                    close(it);
                    FD_CLR(it, &master_set);
                    continue;
                }

                // broadcast the received data
                printf("%d: %s", it, buffer);
                for (broadcast_it = 0; broadcast_it <= select_max; broadcast_it++) {
                    if (!FD_ISSET(broadcast_it, &master_set)) {
                        continue;
                    }

                    if (broadcast_it == it || broadcast_it == server_socket) {
                        continue;
                    }

                    if (send(broadcast_it, buffer, bytes, 0) < -1) {
                        fprintf(stderr, "--- send error: %s\n", strerror(errno));
                    }
                }
            }
        }
    };

    return rc;
}

int main(int argc, char *argv[])
{
    int server_socket;
    int rc = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    server_socket = start_server(argv[1]);
    if (server_socket < 0) {
        return server_socket;
    }

    return server_loop(server_socket, argv[1]);
}

