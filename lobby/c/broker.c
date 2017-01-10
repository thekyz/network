#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "broker.h"

int g_lobby;
/*int g_sink;*/

static int _broadcast_to_lobby(char msg[BROKER_MAX_MSG_LENGTH], int msg_len)
{
    int bytes = nn_send(g_lobby, msg, msg_len, 0);
    assert(bytes == msg_len);

    /*printf("[B] --> %s\n", msg);*/

    return bytes;
}

int main(int argc, char **argv)
{
    g_lobby = nn_socket(AF_SP, NN_PUB);
    assert(g_lobby >= 0);
    assert(nn_bind(g_lobby, BROKER_LOBBY) >= 0);

    printf("[B] +++ Broker started (lobby: %s / sink: %s)" , BROKER_LOBBY, BROKER_SINK);

    for (;;) {
        _broadcast_to_lobby("test", 5);
        sleep(1);
    }


    return 0;
}
