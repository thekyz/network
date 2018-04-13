#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "list.h"
#include "connection.h"
#include "net.h"
#include "server.h"
#include "broker.h"

#define _CLIENT_MODE_IDLE   "idle"
#define _CLIENT_MODE_READY  "ready"

#define err(__m, ...)       fprintf(stderr, "[%s] Error: " __m "\n", g_connection.name, ##__VA_ARGS__);
#define log(__m, ...)       printf("[%s] " __m "\n", g_connection.name, ##__VA_ARGS__);

// connection info
static struct connection g_connection = {0};
static char g_state[NET_MAX_NAME_LENGTH];

static char g_server_sink_addr[NET_MAX_NAME_LENGTH];
static char g_server_pubsub_addr[NET_MAX_NAME_LENGTH];

static int g_server_sink = 0;

// ongoing requests
static bool g_list_clients = false;
static bool g_list_servers = false;

static void _ping()
{
    char connections[NET_MAX_NAME_LENGTH] = "lobby";

    if (g_connection.secondary_connected) {
        snprintf(connections, NET_MAX_NAME_LENGTH, "%s", g_connection.secondary_name);

        net_ping(g_server_sink, g_connection.name, NET_PING_CLIENT, g_state, NET_NA, connections);
    }

    net_ping(g_connection.lobby_sink, g_connection.name, NET_PING_CLIENT, g_state, NET_NA, connections);
}

static void _parse_user_input(const char *input_buffer)
{
    if (input_buffer[0] == '/') {
      #define _str_match(__a, __b)  (strncmp(__a, __b, strlen(__b)) ==  0)

        if (_str_match(input_buffer, "/help")) {
            log("  /help           Print this help message");
            log("  /servers        Show the servers availables");
            log("  /join <s>       Join server <s>");
            log("  /leave          Leave the current server <s>");
            log("  /s <m>          Send the message <m> in the server chatroom");
            log("  /ready          Toggle ready mode (only in server rooms)")
            log("  /clients        Show the clients currently in the lobby (and connected server room)");
            log("  /w <u> <m>      Send the message <m> to client <u>");
        } else if (_str_match(input_buffer, "/clients")) {
            net_list_clients(g_connection.lobby_sink, g_connection.name);
            g_list_clients = true;
        } else if (_str_match(input_buffer, "/servers")) {
            net_list_servers(g_connection.lobby_sink, g_connection.name);
            g_list_servers = true;
        } else if (_str_match(input_buffer, "/leave")) {
            if (g_connection.secondary_connected) {
                g_connection.secondary_poll= false;
                g_connection.secondary_connected = false;
                sprintf(g_connection.secondary_name, "");
            } else {
                err("not connected to a server");
            }
        } else if (_str_match(input_buffer, "/join")) {
            if (g_connection.secondary_connected == false && strlen(g_connection.secondary_name) == 0) {
                char frame_buffer[NET_MAX_MSG_LENGTH];
                snprintf(frame_buffer, NET_MAX_MSG_LENGTH, "%s", input_buffer);

                // discard the command token
                (void)strtok(frame_buffer, " ");

                char *server_name = strtok(NULL, " ");

                net_connect(g_connection.lobby_sink, g_connection.name, server_name);

                snprintf(g_connection.secondary_name, NET_MAX_MSG_LENGTH, "%s", server_name);
            }
        } else if (_str_match(input_buffer, "/s")) {
            if (g_connection.secondary_connected) {
                char frame_buffer[NET_MAX_MSG_LENGTH];
                snprintf(frame_buffer, NET_MAX_MSG_LENGTH, "%s", input_buffer);

                // discard the command token
                (void)strtok(frame_buffer, " ");

                char *msg = strtok(NULL, "");
                net_msg(g_server_sink, g_connection.name, msg);
            } else {
                err("not connected to a server");
            }
        } else if (_str_match(input_buffer, "/w")) {
            char frame_buffer[NET_MAX_MSG_LENGTH];
            snprintf(frame_buffer, NET_MAX_MSG_LENGTH, "%s", input_buffer);

            // discard the command token
            (void)strtok(frame_buffer, " ");

            char *user = strtok(NULL, " ");
            if (user == NULL) {
                err("*** You must provide a client name to send a message to.");
                return;
            }

            char *msg = strtok(NULL, "");
            if (msg == NULL) {
                err("*** You must provide a message to send to '%s'.", user);
                return;
            }

            net_whisper(g_connection.lobby_sink, g_connection.name, user, msg);
        } else if (_str_match(input_buffer, "/ready")) {
            if (g_connection.secondary_connected) {
                if (strcmp(g_state, _CLIENT_MODE_IDLE) == 0) {
                    sprintf(g_state, "%s", _CLIENT_MODE_READY);
                } else {
                    sprintf(g_state, "%s", _CLIENT_MODE_IDLE);
                }

                _ping();
            }
        }
    } else {
        net_msg(g_connection.lobby_sink, g_connection.name, input_buffer);
    }
}

static void _connect(const char *id)
{
    snprintf(g_server_pubsub_addr, NET_MAX_NAME_LENGTH, "tcp://127.0.0.1:%d", SERVER_BASE_PUB_PORT + atoi(id));
    snprintf(g_server_sink_addr, NET_MAX_NAME_LENGTH, "tcp://127.0.0.1:%d", SERVER_BASE_SINK_PORT + atoi(id));

    g_connection.secondary_socket = nn_socket(AF_SP, NN_SUB);
    assert(g_connection.secondary_socket >= 0);
    assert(nn_setsockopt(g_connection.secondary_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
    assert(nn_connect(g_connection.secondary_socket, g_server_pubsub_addr) >= 0);

    g_server_sink = nn_socket(AF_SP, NN_PUSH);
    assert(g_server_sink >= 0);
    assert(nn_connect(g_server_sink, g_server_sink_addr) >= 0);

    g_connection.secondary_poll = true;

    log("--- Connecting to '%s' (%s / %s) ...", g_connection.secondary_name, g_server_pubsub_addr, g_server_sink_addr);
}

static void _read_from_server()
{
    char *data = NULL;
    int bytes = nn_recv(g_connection.secondary_socket, &data, NN_MSG, 0);
    assert(bytes >= 0);

    g_connection.secondary_connected = true;

    /*if (strncmp(strstr(data, NET_RECORD_SEPARATOR) + strlen(NET_RECORD_SEPARATOR), NET_PING, 4) != 0) log("'%s'", data);*/
    /*log("'%s'", data);*/

    char *user = NET_FIRST_TOKEN(data);
    char *cmd = NET_NEXT_TOKEN();

    if (strcmp(cmd, NET_MSG) == 0) {
        char *msg = NET_NEXT_TOKEN();
        printf("<%s> %s: %s\n", g_connection.secondary_name, user, msg);
    }
}

static void _on_msg(char *user, char *data)
{
    printf("<lobby> %s: %s\n", user, data);
}

static void _on_ping(char *user, char *data)
{
    if (strcmp(user, "broker") == 0 ) {
        return;
    }
    if (!g_connection.secondary_connected && !g_connection.secondary_poll && strlen(g_connection.secondary_name) && strcmp(g_connection.secondary_name, user) == 0) {
        char *type = NET_FIRST_TOKEN(data);
        char *state = NET_NEXT_TOKEN();
        char *id = NET_NEXT_TOKEN();
        char *connections = NET_NEXT_TOKEN();

        _connect(id);
    }
}

static void _on_whisp(char *user, char *data)
{
    char *dest = NET_FIRST_TOKEN(data);
    char *msg = NET_NEXT_TOKEN();

    if (strcmp(dest, g_connection.name) == 0) {
        printf("<whisp> %s: %s\n", user, msg);
    }
}

static void _on_info(char *user, char *data)
{
    char *info_type = NET_FIRST_TOKEN(data);

    if (g_list_clients && strcmp(info_type, NET_INFO_CLIENTS) == 0) {
        char *name = NET_NEXT_TOKEN();
        char *state = NET_NEXT_TOKEN();
        char *connections = NET_NEXT_TOKEN();
        log("  c: %s <%s> (%s)", name, state, connections);
    } else if (g_list_servers && strcmp(info_type, NET_INFO_SERVERS) == 0) {
        char *name = NET_NEXT_TOKEN();
        char *state = NET_NEXT_TOKEN();
        char *connections = NET_NEXT_TOKEN();
        log("  s: %s <%s> (%s/%d)", name, state, connections, SERVER_MAX_CLIENTS);
    } else if (strcmp(info_type, NET_INFO_END) == 0) {
        char *end_type = NET_NEXT_TOKEN();
        char *state = NET_NEXT_TOKEN();
        if (strcmp(end_type, NET_INFO_SERVERS) == 0) {
            g_list_servers = false;
        } else if (strcmp(end_type, NET_INFO_CLIENTS) == 0) {
            g_list_clients = false;
        }
    }
}

static void _usage(const char *app)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "   %s <broker_addr> <name>\n", app);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        _usage(argv[0]);
        return -1;
    }

    snprintf(g_connection.name, CONNECTION_NAME_MAX_LENGTH, "%s", argv[2]);
    snprintf(g_state, NET_MAX_NAME_LENGTH, _CLIENT_MODE_IDLE);

    g_connection.on_msg = _on_msg;
    g_connection.on_ping = _on_ping;
    g_connection.on_info = _on_info;
    g_connection.on_whisp = _on_whisp;
    g_connection.on_input = _parse_user_input;
    g_connection.on_secondary = _read_from_server;
    g_connection.ping = _ping;
    g_connection.secondary_poll = false;

    connection_poll(&g_connection, argv[1]);
    connection_cleanup(&g_connection);

    return 0;
}
