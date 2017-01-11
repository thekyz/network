#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "broker.h"

int g_lobby;
int g_sink;

static int _whisper(const char *from, const char *to, const char *msg)
{
    return BROKER_SEND(g_lobby, BROKER_WHISP_FORMAT(from, to, msg));
}

static int _broadcast(const char *user, const char *msg)
{
    return BROKER_SEND(g_lobby, BROKER_MSG_FORMAT(user, msg));
}

static void _cleanup()
{
    nn_shutdown(g_sink, 0);
    nn_shutdown(g_lobby, 0);

    exit(0);
}

static void _int_handler(int dummy)
{
    _cleanup();
}

static void _read_from_sink()
{
    char *data = NULL;
    int bytes = nn_recv(g_sink, &data, NN_MSG, 0);
    assert(bytes >= 0);

    /*printf("'%s'\n", data);*/

    char *user = strtok(data, BROKER_UNIT_SEPARATOR);
    char *cmd = strtok(NULL, BROKER_UNIT_SEPARATOR);

    if (strcmp(cmd, BROKER_MSG) == 0) {
        char *msg = strtok(NULL, BROKER_UNIT_SEPARATOR);
        printf("[B] %s: '%s'\n", user, msg);
        _broadcast(user, msg);
    } else if (strcmp(cmd, BROKER_WHISP) == 0) {
        char *dest = strtok(NULL, BROKER_UNIT_SEPARATOR); 
        char *msg = strtok(NULL, BROKER_UNIT_SEPARATOR); 
        _whisper(user, dest, msg);
    }

    nn_freemsg(data);
}

int main(int argc, char **argv)
{
    signal(SIGINT, _int_handler);

    g_lobby = nn_socket(AF_SP, NN_PUB);
    assert(g_lobby >= 0);
    assert(nn_bind(g_lobby, BROKER_LOBBY) >= 0);

    g_sink = nn_socket(AF_SP, NN_PULL);
    assert(g_sink >= 0);
    assert(nn_bind(g_sink, BROKER_SINK) >= 0);

    printf("[B] +++ Broker started (lobby: %s / sink: %s)\n" , BROKER_LOBBY, BROKER_SINK);

    for (;;) {
        _read_from_sink();
    }

    _cleanup();

    return 0;
}

