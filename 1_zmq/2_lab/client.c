#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "net.h"

#define __PP(__out, __fmt, ...)     fprintf(__out, "[%s] " __fmt, player_name, ## __VA_ARGS__)
#define PP(__fmt, ...)              __PP(stdout, __fmt, ## __VA_ARGS__) 
#define PPE(__fmt, ...)             __PP(stderr, __fmt, ## __VA_ARGS__) 

int main(int argc, char *argv[])
{
    void *ctx = zmq_ctx_new();
    char *player_name = argv[1];

    PP("Connecting to server ...\n");
    void *lobby = zmq_socket(ctx, ZMQ_PUSH);
    zmq_connect(lobby, "tcp://localhost:" LOBBY_IN_PORT);

    char msg[MAX_MSG_LEN] = "";
    sprintf(msg, A_CONNECTED "%s", player_name);
    zmq_send(lobby, msg, strlen(msg) + 1, 0);

    sleep(1);
    sprintf(msg, A_READY "%s", player_name);
    zmq_send(lobby, msg, strlen(msg) + 1, 0);

    return 0;
}
