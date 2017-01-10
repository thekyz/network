#include <stdio.h>
#include <assert.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "broker.h"

char * g_name;
int g_lobby_sub;
/*int g_lobby_sink;*/

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "[C] Usage: ./%s <name>\n", argv[0]);
        return -1;
    }

    g_name = argv[1];

    g_lobby_sub = nn_socket(AF_SP, NN_SUB);
    assert(g_lobby_sub >= 0);
    assert(nn_setsockopt(g_lobby_sub, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
    assert(nn_connect(g_lobby_sub, BROKER_LOBBY) >= 0);

    for (;;) {
        char *msg = NULL;
        int bytes = nn_recv(g_lobby_sub, &msg, NN_MSG, 0);
        assert(bytes >= 0);
        printf("[C] lobby: %s\n", msg);
        nn_freemsg(msg);
    }

    nn_shutdown(g_lobby_sub, 0);

    return 0;
}
