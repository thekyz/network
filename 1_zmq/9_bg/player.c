#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "dealer_net.h"

int main(int argc, char *argv[])
{
    void *ctx = zmq_ctx_new();

    printf("[C] Connecting to server ...\n");
    void *lobby = zmq_socket(ctx, ZMQ_PUSH);
    zmq_connect(lobby, "tcp://localhost:" DEALER_LOBBY_PORT);

    char msg[DEALER_MAX_MSG_LEN] = "";
    sprintf(msg, DEALER_LOBBY_PLAYER_CONNECTED "%s", "thekyz");
    zmq_send(lobby, msg, strlen(msg) + 1, 0);

    sleep(1);
    sprintf(msg, DEALER_LOBBY_PLAYER_READY "%s", "thekyz");
    zmq_send(lobby, msg, strlen(msg) + 1, 0);

    void *table = zmq_socket(ctx, ZMQ_SUB);
    zmq_connect(table, "tcp://localhost:" DEALER_TABLE_PORT);

    const char *msg_filter = DEALER_TABLE_FILTER_MSG;
    zmq_setsockopt(table, ZMQ_SUBSCRIBE, DEALER_TABLE_FILTER_MSG, strlen(DEALER_TABLE_FILTER_MSG));
    zmq_setsockopt(table, ZMQ_SUBSCRIBE, DEALER_TABLE_FILTER_GAME, strlen(DEALER_TABLE_FILTER_GAME));

    for (;;) {
        memset(msg, 0, sizeof(msg));
        zmq_recv(table, msg, DEALER_MAX_MSG_LEN, 0);
        msg[DEALER_MAX_MSG_LEN - 1] = '\0';

        if (strncmp(msg, DEALER_TABLE_FILTER_MSG, strlen(DEALER_TABLE_FILTER_MSG)) == 0) {
            printf("[C] msg << %s\n", &msg[strlen(DEALER_TABLE_FILTER_MSG)]);
        } else if (strncmp(msg, DEALER_TABLE_FILTER_GAME, strlen(DEALER_TABLE_FILTER_GAME)) == 0) {
            printf("[C] game << %s\n", &msg[strlen(DEALER_TABLE_FILTER_GAME)]);
        } else {
            fprintf(stderr, "[C] Error: unknown message filter on table socket !\n");
        }
    }

    return 0;
}
