#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "net.h"
#include "log.h"
#include "broker.h"
#include "list.h"
#include "connection.h"

#define err(__m, ...)       fprintf(stderr, "[%s] Error: " __m "\n", conn->name, ##__VA_ARGS__);
#define log(__m, ...)       printf("[%s] " __m "\n", conn->name, ##__VA_ARGS__);

static const int g_max_input_length = 256;      // Max number of chars read from the input

static struct connection *g_connection;

void connection_cleanup(struct connection *conn)
{
    // spam the shutdown message a bit ...
    for (int i = 0; i < 5; i++) {
        net_shutdown(conn->lobby_sink, conn->name);
    }

    usleep(10000);

    nn_shutdown(conn->lobby_pubsub, 0);
    nn_shutdown(conn->lobby_sink, 0);

    log("--- Shutting down ...");

    exit(0);
}

static void _read_from_lobby(struct connection *conn)
{
    char *msg = NULL;
    int bytes = nn_recv(conn->lobby_pubsub, &msg, NN_MSG, 0);
    assert(bytes >= 0);

    conn->lobby_connected = true;

    /*if (strncmp(strstr(msg, NET_RECORD_SEPARATOR) + strlen(NET_RECORD_SEPARATOR), NET_PING, 4) != 0) log("'%s'", msg);*/
    /*log("'%s'", msg);*/

    char *user = NET_FIRST_TOKEN(msg);
    char *cmd = NET_NEXT_TOKEN();
    char *data = NET_LAST_TOKEN();

    if (strcmp(cmd, NET_PING) == 0) {
        if (conn->on_ping) {
            conn->on_ping(user, data);
        }
    } else if (strcmp(cmd, NET_MSG) == 0) {
        if (conn->on_msg) {
            conn->on_msg(user, data);
        }
    } else if (strcmp(cmd, NET_WHISP) == 0) {
        if (conn->on_whisp) {
            conn->on_whisp(user, data);
        }
    } else if (strcmp(cmd, NET_INFO) == 0) {
        if (conn->on_info) {
            conn->on_info(user, data);
        }
    } else if (strcmp(cmd, NET_SHUTDOWN) == 0) {
        if (strcmp(user, BROKER_NAME) == 0) {
            conn->lobby_connected = false;
        }

        if (conn->on_shutdown) {
            conn->on_shutdown(user, data);
        }
    }

    nn_freemsg(msg);
}

static void _control(struct connection *conn)
{
    if (conn->ping) {
        conn->ping();
    }

    if (conn->on_control) {
        conn->on_control();
    }
}

static ptrdiff_t _strn_trim(char *str, ptrdiff_t max_chars)
{
    char *c = str;
    char *last = c;

    while ((*c != '\0') && ((c - str) < max_chars)) {
        if (!isspace(*c)) {
            last = c;
        }

        c++;
    }

    *(++last) = '\0';

    return (c - last);
}

static int _get_user_input(char *buffer, size_t buffer_length)
{
    if (fgets(buffer, buffer_length, stdin) != buffer) {
        return -1;
    }

    char *endline = strchr(buffer, '\n');
    if (endline) {
        // endline captured, remove it from string
        *endline = '\0';
    } else {

        // endline not captured, flush stdin ...
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
    }

    // TODO use ncurses to NOT deal with all of this ...

    _strn_trim(buffer, g_max_input_length);
    if (strlen(buffer) == 0) {
        return -1;
    }

    return 0;
}

void connection_poll(struct connection *conn, const char *broker_addr)
{
    log("--- Connecting to broker (%s) as '%s' ...", broker_addr, conn->name);
    conn->lobby_connected = false;

    char broker_lobby_addr[NET_MAX_NAME_LENGTH];
    snprintf(broker_lobby_addr, NET_MAX_NAME_LENGTH, "tcp://%s:%s", broker_addr, BROKER_LOBBY_PORT);
    char broker_sink_addr[NET_MAX_NAME_LENGTH];
    snprintf(broker_sink_addr, NET_MAX_NAME_LENGTH, "tcp://%s:%s", broker_addr, BROKER_SINK_PORT);

    conn->lobby_pubsub = nn_socket(AF_SP, NN_SUB);
    assert(conn->lobby_pubsub >= 0);
    assert(nn_setsockopt(conn->lobby_pubsub, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
    assert(nn_connect(conn->lobby_pubsub, broker_lobby_addr) >= 0);

    conn->lobby_sink = nn_socket(AF_SP, NN_PUSH);
    assert(conn->lobby_sink >= 0);
    assert(nn_connect(conn->lobby_sink, broker_sink_addr) >= 0);

    struct pollfd nodes[4] = { 0 };

    // stdin
    nodes[0].events = POLLIN;

    // broker lobby
    size_t fd_size = sizeof(nodes[1].fd);
    assert(nn_getsockopt(conn->lobby_pubsub, NN_SOL_SOCKET, NN_RCVFD, &nodes[1].fd, &fd_size) == 0);
    nodes[1].events = POLLIN;

    // hearthbeat
    struct itimerspec ts;
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    assert(tfd != -1);
    nodes[2].events = POLLIN;

    ts.it_interval.tv_sec = BROKER_KEEP_ALIVE_PERIOD;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 1;

    char input_buffer[g_max_input_length];
    bool lobby_connected = false;
    bool secondary_connected = false;
    bool secondary_poll = false;
    for (;;) {
        if (lobby_connected && !conn->lobby_connected) {
            log("--- Lost connection to lobby, reconnecting ...");

            // disable polling on input
            nodes[0].fd = 0;
            // disable hearthbeat timer
            nodes[2].fd = 0;
        } else if (!lobby_connected && conn->lobby_connected) {
            static bool print_once = true;
            log("+++ Connected to lobby !");
            if (conn->on_input) {
                if (print_once) {
                    log("Type '/help' for a list of commands.");
                    print_once = false;
                }

                // enable polling on input
                nodes[0].fd = STDIN_FILENO;
            }

            // enable hearthbeat timer
            nodes[2].fd = tfd;
            assert(timerfd_settime(nodes[2].fd, 0, &ts, NULL) == 0);
        }

        lobby_connected = conn->lobby_connected;

        if (secondary_connected && !conn->secondary_connected) {
            log("--- Lost connection to '%s'", conn->secondary_name);
        } else if (!secondary_connected && conn->secondary_connected) {
            log("+++ Connected to '%s' !", conn->secondary_name);
        }

        secondary_connected = conn->secondary_connected;

        if (!secondary_poll && conn->secondary_poll) {
            size_t secondary_fd_size = sizeof(nodes[3].fd);
            assert(nn_getsockopt(conn->secondary_socket, NN_SOL_SOCKET, NN_RCVFD, &nodes[3].fd, &secondary_fd_size) == 0);
            nodes[3].events = POLLIN;
        } else if (secondary_poll && !conn->secondary_poll) {
            nodes[3].events = 0;
        }

        secondary_poll = conn->secondary_poll;

        int rc = poll(nodes, sizeof(nodes) / sizeof(struct pollfd), -1);
        if (rc == -1) {
            err("poll() error: %s", strerror(errno));

            return;
        } else if (rc == 0) {
            // timeout
            continue;
        }

        if (nodes[0].revents & POLLIN) {
            memset(input_buffer, 0, sizeof(input_buffer));
            if (_get_user_input(input_buffer, sizeof(input_buffer)) == 0) {
                conn->on_input(input_buffer);
            }
        }

        if (nodes[1].revents & POLLIN) {
            _read_from_lobby(conn);
        }

        if (nodes[2].revents & POLLIN) {
            uint64_t res;
            int rc = read(nodes[2].fd, &res, sizeof(res));
            if (rc == -1 && errno != EAGAIN) {
                err("read() error on timerfd: %s", strerror(errno));
            } else {
                _control(conn);
            }
        }

        if (nodes[3].revents & POLLIN) {
            conn->on_secondary();
        }
    }
}
