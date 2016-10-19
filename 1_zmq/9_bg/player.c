#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "dealer_net.h"

#define __PP(__out, __fmt, ...)     fprintf(__out, "[%s] " __fmt, player_name, ## __VA_ARGS__)
#define PP(__fmt, ...)              __PP(stdout, __fmt, ## __VA_ARGS__) 
#define PPE(__fmt, ...)             __PP(stderr, __fmt, ## __VA_ARGS__) 

int main(int argc, char *argv[])
{
    void *ctx = zmq_ctx_new();
    char *player_name = argv[1];

    PP("Connecting to server ...\n");
    void *lobby = zmq_socket(ctx, ZMQ_PUSH);
    zmq_connect(lobby, "tcp://localhost:" DEALER_LOBBY_PORT);

    char msg[MAX_MSG_LEN] = "";
    sprintf(msg, A_CONNECTED "%s", player_name);
    zmq_send(lobby, msg, strlen(msg) + 1, 0);

    sleep(1);
    sprintf(msg, A_READY "%s", player_name);
    zmq_send(lobby, msg, strlen(msg) + 1, 0);

    void *table = zmq_socket(ctx, ZMQ_SUB);
    zmq_connect(table, "tcp://localhost:" DEALER_TABLE_PORT);

    const char *msg_filter = F_MSG;
    zmq_setsockopt(table, ZMQ_SUBSCRIBE, F_MSG, strlen(F_MSG));
    zmq_setsockopt(table, ZMQ_SUBSCRIBE, F_GAME, strlen(F_GAME));

    for (;;) {
        memset(msg, 0, sizeof(msg));
        zmq_recv(table, msg, MAX_MSG_LEN, 0);
        msg[MAX_MSG_LEN - 1] = '\0';

      #define IS_MSG(__m) strncmp(msg, __m, strlen(__m)) == 0
        if (IS_MSG(F_MSG)) {
            PP("[D] %s\n", &msg[strlen(F_MSG)]);
        } else if (IS_MSG(F_GAME)) {
            char *split = &msg[strlen(F_GAME)];
            PP("game << %s\n", split);

          #define IS_SPLIT(__m) strncmp(split, __m, strlen(__m)) == 0
            if (IS_SPLIT(G_BET)) {
                char bet[MAX_MSG_LEN] = "";
                sprintf(bet, A_PLAY "%s %s %s", player_name, G_BET, "10");
                /*printf("[C] bet >> %s\n", bet);*/
                zmq_send(lobby, bet, strlen(bet) + 1, 0);
            }
        } else {
            PPE("Error: unknown message filter on table socket !\n");
        }
    }

    return 0;
}
