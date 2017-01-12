#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <stdbool.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "broker.h"
#include "list.h"

static const int g_max_input_length = 256;      // Max number of chars read from the input

// sockets
static int g_lobby;
static int g_sink;

struct _connection {
    list node;
    char name[BROKER_MAX_NAME_LENGTH];
    int alive;
};

static list g_clients;
static list g_servers;

static int _whisper(const char *from, const char *to, const char *msg)
{
    return BROKER_SEND(g_lobby, BROKER_WHISP_FORMAT(from, to, msg));
}

static int _broadcast(const char *user, const char *msg)
{
    return BROKER_SEND(g_lobby, BROKER_MSG_FORMAT(user, msg));
}

static int _ping()
{
    return BROKER_SEND(g_lobby, BROKER_PING_FORMAT(BROKER_PING_BROKER, BROKER_PING_BROKER));
}

static int _send_connection_list(const char *user, const char *type)
{
    char *info_type = NULL;
    list *conn_list = NULL;

    if (strcmp(type, BROKER_LIST_CLIENTS) == 0) {
        info_type = BROKER_INFO_CLIENTS;
        conn_list = &g_clients;
    } else if (strcmp(type, BROKER_LIST_SERVERS) == 0) {
        info_type = BROKER_INFO_SERVERS;
        conn_list = &g_servers;
    }

    struct _connection *conn;
    size_t list_msg_size = BROKER_LIST_MAX_SIZE(user, type);
    char list_msg[list_msg_size];
    int list_msg_index = 0;
    list_foreach(conn_list, conn) {
        if (list_msg_index + strlen(BROKER_UNIT_SEPARATOR) + strlen(conn->name) + 1 > list_msg_size) {
            // send what we have and prepare the next message
            BROKER_SEND(g_lobby, BROKER_INFO_FORMAT(user, info_type, list_msg));
            list_msg_index = 0;
            list_msg[0] = '\0';
        }

        list_msg_index += sprintf(&list_msg[list_msg_index], "%s", conn->name);

        if (conn->node.next != conn_list) {
            list_msg_index += sprintf(&list_msg[list_msg_index], "%s", BROKER_UNIT_SEPARATOR);
        }
    }

    return BROKER_SEND(g_lobby, BROKER_INFO_FORMAT(user, info_type, list_msg));
}

static void _cleanup()
{
    BROKER_SEND(g_lobby, BROKER_SHUTDOWN_FORMAT(BROKER_NAME));

    nn_shutdown(g_sink, 0);
    nn_shutdown(g_lobby, 0);

    unlink(BROKER_SINK_FILE);
    unlink(BROKER_LOBBY_FILE);

    exit(0);
}

static void _int_handler(int dummy)
{
    _cleanup();
}

static void _hearthbeat(const char *conn_type, const char *name)
{
    list *conn_list = NULL;
    if (strcmp(conn_type, BROKER_PING_SERVER) == 0) {
        conn_list = &g_servers;
    } else if (strcmp(conn_type, BROKER_PING_CLIENT) == 0) {
        conn_list = &g_clients;
    }

    struct _connection *conn;
    list_foreach(conn_list, conn) {
        if (strcmp(conn->name, name) == 0) {
            // found it: keep alive
            conn->alive = 2;
            return;
        }
    }

    conn = (struct _connection *)malloc(sizeof(struct _connection));
    sprintf(conn->name, "%s", name);
    conn->alive = 2;
    list_add_tail(conn_list, &conn->node);
}

static void _check_connections(list *conn_list)
{
    struct _connection *conn;
    list_foreach(conn_list, conn) {
        conn->alive--;

        if (conn->alive == 0) {
            list_delete(&conn->node);
            free(conn);
        }
    }
}

static void _read_from_sink()
{
    char *data = NULL;
    int bytes = nn_recv(g_sink, &data, NN_MSG, 0);
    assert(bytes >= 0);

    /*if (strncmp(strstr(data, BROKER_RECORD_SEPARATOR) + strlen(BROKER_RECORD_SEPARATOR), BROKER_PING, 4) != 0) printf("'%s'\n", data);*/

    char *user = BROKER_FIRST_TOKEN(data);
    char *cmd = BROKER_NEXT_TOKEN();

    if (strcmp(cmd, BROKER_PING) == 0) {
        char *user_type = BROKER_NEXT_TOKEN();
        _hearthbeat(user_type, user);
    } else if (strcmp(cmd, BROKER_MSG) == 0) {
        char *msg = BROKER_NEXT_TOKEN();
        printf("[B] %s: '%s'\n", user, msg);
        _broadcast(user, msg);
    } else if (strcmp(cmd, BROKER_WHISP) == 0) {
        char *dest = BROKER_NEXT_TOKEN();
        char *msg = BROKER_NEXT_TOKEN();
        _whisper(user, dest, msg);
    } else if (strcmp(cmd, BROKER_LIST) == 0) {
        char *type = BROKER_NEXT_TOKEN();
        _send_connection_list(user, type);
    }

    nn_freemsg(data);
}

static int _poll()
{
    struct pollfd nodes[4];

    // stdin, provisioned, not used yet
    nodes[0].fd = 0;//STDIN_FILENO;
    nodes[0].events = POLLIN;

    // broker sink
    size_t fd_size = sizeof(nodes[1].fd);
    assert(nn_getsockopt(g_sink, NN_SOL_SOCKET, NN_RCVFD, &nodes[1].fd, &fd_size) == 0);
    nodes[1].events = POLLIN;

    // keep alive
    struct itimerspec keep_alive_ts;
    nodes[2].fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    nodes[2].events = POLLIN;
    assert(nodes[2].fd != -1);

    keep_alive_ts.it_interval.tv_sec = BROKER_KEEP_ALIVE_PERIOD;
    keep_alive_ts.it_interval.tv_nsec = 0;
    keep_alive_ts.it_value.tv_sec = 0;
    keep_alive_ts.it_value.tv_nsec = 1;
    assert(timerfd_settime(nodes[2].fd, 0, &keep_alive_ts, NULL) == 0);

    // hearthbeat
    struct itimerspec hearthbeat_ts;
    nodes[3].fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    nodes[3].events = POLLIN;
    assert(nodes[3].fd != -1);

    hearthbeat_ts.it_interval.tv_sec = BROKER_HEARTHBEAT_PERIOD;
    hearthbeat_ts.it_interval.tv_nsec = 0;
    hearthbeat_ts.it_value.tv_sec = 0;
    hearthbeat_ts.it_value.tv_nsec = 1;
    assert(timerfd_settime(nodes[3].fd, 0, &hearthbeat_ts, NULL) == 0);

    for (;;) {
        char input_buffer[g_max_input_length];
        int rc = poll(nodes, sizeof(nodes) / sizeof(struct pollfd), -1);

        if (rc == -1) {
            fprintf(stderr, "[C] poll() error: %s\n", strerror(errno));
            return -1;
        } else if (rc == 0) {
            // timeout
            continue;
        }

        if (nodes[0].revents & POLLIN) {
            // no input parser for now ...
        }

        if (nodes[1].revents & POLLIN) {
            _read_from_sink();
        }

        if (nodes[2].revents & POLLIN) {
            uint64_t res;
            int rc = read(nodes[2].fd, &res, sizeof(res));
            if (rc == -1 && errno != EAGAIN) {
                fprintf(stderr, "[B] read() error on timerfd: %s\n", strerror(errno));
            } else {
                _check_connections(&g_servers);
                _check_connections(&g_clients);
            }
        }

        if (nodes[3].revents & POLLIN) {
            uint64_t res;
            int rc = read(nodes[3].fd, &res, sizeof(res));
            if (rc == -1 && errno != EAGAIN) {
                fprintf(stderr, "[B] read() error on timerfd: %s\n", strerror(errno));
            } else {
                _ping();
            }
        }
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, _int_handler);

    list_init(&g_clients);
    list_init(&g_servers);

    g_lobby = nn_socket(AF_SP, NN_PUB);
    assert(g_lobby >= 0);
    assert(nn_bind(g_lobby, BROKER_LOBBY) >= 0);

    g_sink = nn_socket(AF_SP, NN_PULL);
    assert(g_sink >= 0);
    assert(nn_bind(g_sink, BROKER_SINK) >= 0);

    printf("[B] +++ Broker started (lobby: %s / sink: %s)\n" , BROKER_LOBBY, BROKER_SINK);

    _poll();
    _cleanup();

    return 0;
}

