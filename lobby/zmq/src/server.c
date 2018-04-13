#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include <zmq.h>

#include "list.h"
#include "connection.h"
#include "net.h"
#include "log.h"
#include "broker.h"
#include "server.h"

#define _SERVER_MODE_GAME   "game"
#define _SERVER_MODE_CHAT   "chat"

#define err(__m, ...)       fprintf(stderr, "[%s] Error: " __m "\n", g_connection.name, ##__VA_ARGS__);
#define log(__m, ...)       printf("[%s] " __m "\n", g_connection.name, ##__VA_ARGS__);

// connection info
static struct connection g_connection = {0};
static char g_state[NET_MAX_NAME_LENGTH];
static char g_id[NET_MAX_NAME_LENGTH];

static char g_broker_lobby_addr[NET_MAX_NAME_LENGTH];
static char g_broker_sink_addr[NET_MAX_NAME_LENGTH];
static char g_server_sink_addr[NET_MAX_NAME_LENGTH];
static char g_server_pubsub_addr[NET_MAX_NAME_LENGTH];

// sockets
static void *g_server_pubsub = 0;

static list g_connected_clients;
static bool g_shutdown_validate = false;
static unsigned int g_shutdown_check = 0;
static char g_server_sink_port[NET_MAX_NAME_LENGTH];
static char g_server_pubsub_port[NET_MAX_NAME_LENGTH];

static void _ping()
{
    char connections[NET_MAX_NAME_LENGTH] = "";
    sprintf(connections, "%d", list_count(&g_connected_clients));

    net_ping(g_server_pubsub, g_connection.name, NET_PING_SERVER, g_state, g_id, connections);
    net_ping(g_connection.lobby_sink, g_connection.name, NET_PING_SERVER, g_state, g_id, connections);
}

static void _cleanup()
{
}

static void _on_connect(struct net_client *net_client)
{
    log("==> '%s' connected", net_client->name);
}

static void _read_from_sink()
{
    char data[NET_MAX_MSG_LENGTH];
    int bytes = zmq_recv(g_connection.secondary_socket, &data, NET_MAX_MSG_LENGTH, 0);
    assert(bytes >= 0);

    /*if (strncmp(strstr(data, NET_RECORD_SEPARATOR) + strlen(NET_RECORD_SEPARATOR), NET_PING, 4) != 0) log("'%s'", data);*/
    /*log("'%s'", data);*/

    char *user = NET_FIRST_TOKEN(data);
    char *cmd = NET_NEXT_TOKEN();

    if (strcmp(cmd, NET_PING) == 0) {
        char *user_type = NET_NEXT_TOKEN();
        char *state = NET_NEXT_TOKEN();
        char *id = NET_NEXT_TOKEN();
        char *connections = NET_NEXT_TOKEN();
        net_hearthbeat(&g_connected_clients, user, state, id, connections, _on_connect);
    } else if (strcmp(cmd, NET_MSG) == 0) {
        char *msg = NET_NEXT_TOKEN();
        log("%s: '%s'", user, msg);
        net_msg(g_server_pubsub, user, msg);
    }
}

static void _on_shutdown(char *user, char *data)
{
    if (strcmp(user, NET_SHUTDOWN_SERVERS) == 0) {
        if (g_shutdown_validate) {
            _cleanup();
        } else if (g_shutdown_check > BROKER_SERVER_SHUTDOWN_CHECK_PERIOD) {
            g_shutdown_validate = true;
        } else {
            g_shutdown_validate = false;
        }
    }
}

static void _on_disconnect(struct net_client *net_client)
{
    log("--- '%s' disconnected", net_client->name);
}

static void _on_control()
{
   net_check_connections(&g_connected_clients, _on_disconnect);

   if (list_count(&g_connected_clients) == 0) {
       g_shutdown_check += BROKER_KEEP_ALIVE_PERIOD;
   } else {
       g_shutdown_check = 0;
   }
}

static void _usage(const char *app)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "   %s <broker_addr> <name> <id>\n", app);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        _usage(argv[0]);
        return -1;
    }

    snprintf(g_connection.name, CONNECTION_NAME_MAX_LENGTH, "%s", argv[3]);

    sprintf(g_state, _SERVER_MODE_CHAT);
    sprintf(g_id, "%s", argv[3]);

    list_init(&g_connected_clients);

    int server_id = atoi(g_id);

    g_connection.zmq_context = zmq_ctx_new();

    g_server_pubsub = zmq_socket(g_connection.zmq_context, ZMQ_PUB);
    assert(g_server_pubsub >= 0);
    sprintf(g_server_pubsub_addr, "tcp://*:%d", SERVER_BASE_PUB_PORT + server_id);
    if (zmq_bind(g_server_pubsub, g_server_pubsub_addr) < 0) {
        err("Fatal: could not bind pub '%s'", g_server_pubsub_addr);
        return -1;
    }

    g_connection.secondary_socket = zmq_socket(g_connection.zmq_context, ZMQ_PULL);
    assert(g_connection.secondary_socket >= 0);
    sprintf(g_server_sink_addr, "tcp://*:%d", SERVER_BASE_SINK_PORT + server_id);
    if (zmq_bind(g_connection.secondary_socket, g_server_sink_addr) < 0) {
        err("Fatal: could not bind sink '%s'", g_server_sink_addr);
        return -1;
    }

    log("+++ Started (lobby: %s / sink: %s)" , g_server_pubsub_addr, g_server_sink_addr);

    g_connection.on_shutdown = _on_shutdown;
    g_connection.on_secondary = _read_from_sink;
    g_connection.secondary_poll = true;
    g_connection.on_control = _on_control;
    g_connection.ping = _ping;

    connection_poll(&g_connection, argv[1]);
    _cleanup();

    return 0;
}
