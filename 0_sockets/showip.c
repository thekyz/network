#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char **argv)
{
    struct addrinfo hints;
    struct addrinfo *serverInfo;
    char ipString[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(argv[1], NULL, &hints, &serverInfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for '%s': \n", argv[1]);

    struct addrinfo *it;
    for (it = serverInfo; it != NULL; it = it->ai_next) {
        void *address;
        char *ipVersion;

        if (it->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)it->ai_addr;
            address = &(ipv4->sin_addr);
            ipVersion = "IPv4";
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)it->ai_addr;
            address = &(ipv6->sin6_addr);
            ipVersion = "IPv6";
        }

        inet_ntop(it->ai_family, address, ipString, sizeof(ipString));
        printf("\t%s: '%s'\n", ipVersion, ipString);
    }

    freeaddrinfo(serverInfo);

    return 0;
}
