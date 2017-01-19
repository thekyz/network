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

#include "net.h"
#include "log.h"
#include "broker.h"
#include "list.h"
#include "system.h"

#define err(__m, ...)		fprintf(stderr, "[%s] Error: " __m "\n", g_name, ##__VA_ARGS__);
#define log(__m, ...)		printf("[%s] " __m "\n", g_name, ##__VA_ARGS__);

static const char g_name[] = BROKER_NAME;

// sockets
static int g_lobby;
static int g_sink;

static list g_clients;
static list g_servers;

static int g_next_server_id = 0;

static int _send_connection_list(const char *user, const char *type)
{
    char *info_type = NULL;
    list *conn_list = NULL;

    if (strcmp(type, NET_LIST_CLIENTS) == 0) {
        info_type = NET_INFO_CLIENTS;
        conn_list = &g_clients;
    } else if (strcmp(type, NET_LIST_SERVERS) == 0) {
        info_type = NET_INFO_SERVERS;
        conn_list = &g_servers;
    }

    struct net_client *conn;
    list_foreach(conn_list, conn) {
		net_info(g_lobby, user, info_type, conn->name, conn->state, conn->connections);
    }

	return net_info(g_lobby, user, NET_INFO_END, info_type, "-", "-");
}

static void _cleanup()
{
	// spam the shutdown message a bit ...
	for (int i = 0; i < 5; i++) {
	    net_shutdown(g_lobby, BROKER_NAME);
	}

	usleep(10000);

    nn_shutdown(g_sink, 0);
    nn_shutdown(g_lobby, 0);

    exit(0);
}

static void _int_handler(int dummy)
{
    _cleanup();
}

static void _on_connect(struct net_client *net_client)
{
    log("==> '%s' connected", net_client->name);
}

static void _hearthbeat(const char *conn_type, const char *name, const char *state, const char *id, const char *connections)
{
    list *conn_list = NULL;
    if (strcmp(conn_type, NET_PING_SERVER) == 0) {
        conn_list = &g_servers;
    } else if (strcmp(conn_type, NET_PING_CLIENT) == 0) {
        conn_list = &g_clients;
    }

    net_hearthbeat(conn_list, name, state, id, connections, _on_connect);
}

static void _connect_client_to_server(const char *client, const char *server)
{
    struct net_client *conn;
    list_foreach(&g_servers, conn) {
        if (strncmp(conn->name, server, strlen(conn->name)) == 0 && conn->alive > 0) {
            // server looks alive
            net_ping(g_lobby, server, NET_PING_SERVER, conn->state, conn->id, "-");
        }
    }
}

static void _read_from_sink()
{
    char *data = NULL;
    int bytes = nn_recv(g_sink, &data, NN_MSG, 0);
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
        _hearthbeat(user_type, user, state, id, connections);
    } else if (strcmp(cmd, NET_CONNECT) == 0) {
        char *server = NET_NEXT_TOKEN();
        _connect_client_to_server(user, server);
    } else if (strcmp(cmd, NET_MSG) == 0) {
        char *msg = NET_NEXT_TOKEN();
        log("%s: '%s'", user, msg);
        net_msg(g_lobby, user, msg);
    } else if (strcmp(cmd, NET_WHISP) == 0) {
        char *dest = NET_NEXT_TOKEN();
        char *msg = NET_NEXT_TOKEN();
        net_whisper(g_lobby, user, dest, msg);
    } else if (strcmp(cmd, NET_LIST) == 0) {
        char *type = NET_NEXT_TOKEN();
        _send_connection_list(user, type);
    }

    nn_freemsg(data);
}

static int _spawn_server()
{
    int server_id = g_next_server_id++;

    char server_name[NET_MAX_NAME_LENGTH];
    sprintf(server_name, "%s%03d", "server-", server_id);
    char server_id_str[NET_MAX_NAME_LENGTH];
    sprintf(server_id_str, "%d", server_id); 

    char *args[] = { "./server", "127.0.0.1", server_name, server_id_str, NULL };
    int rc = exec_cmd(args);
    if (rc == -1) {
        err("Failed to spawn server '%s'", server_name);
        return -1;
    }

    return 0;
}

static void _server_control()
{
    // check if we need to spawn a new server
    int n_clients = list_count(&g_clients);
    int n_servers = list_count(&g_servers);

    int n_spawns = (n_clients / 2) + 1;
    if (n_spawns > BROKER_MAX_SPAWNED_SERVERS) {
        n_spawns = BROKER_MAX_SPAWNED_SERVERS;
    }

    if (n_servers < n_spawns) {
        // spawn more
        for (int i = 0; i < n_spawns - n_servers; i++) {
            _spawn_server();
        }
    }

    if (n_servers > n_spawns) {
        // make a shutdown request (servers empty for more than 30 seconds will shutdown on second request)
        net_shutdown(g_lobby, NET_SHUTDOWN_SERVERS);
    }
}

static void _on_disconnect(struct net_client *net_client)
{
    log("--- '%s' disconnected", net_client->name);
}

static void _control()
{
    net_check_connections(&g_servers, _on_disconnect);
    net_check_connections(&g_clients, _on_disconnect);

    _server_control();
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
        int rc = poll(nodes, sizeof(nodes) / sizeof(struct pollfd), -1);

        if (rc == -1) {
            err("poll(): %s", strerror(errno));
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
                err("read() error on timerfd: %s", strerror(errno));
            } else {
                _control();
            }
        }

        if (nodes[3].revents & POLLIN) {
            uint64_t res;
            int rc = read(nodes[3].fd, &res, sizeof(res));
            if (rc == -1 && errno != EAGAIN) {
                err("read() error on timerfd: %s", strerror(errno));
            } else {
                // ping our presence on the lobby
                net_ping(g_lobby, BROKER_NAME, NET_PING_BROKER, "-", "-", "-");
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

    log("+++ Started (lobby: %s / sink: %s)" , BROKER_LOBBY, BROKER_SINK);

    _poll();
    _cleanup();

    return 0;
}

