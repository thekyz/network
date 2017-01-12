#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <stdbool.h>
#include <ctype.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "broker.h"

static const int g_max_input_length = 256;      // Max number of chars read from the input

// client name
static char *g_name;
static int g_name_len = 0;

// sockets
static int g_lobby_sub;
static int g_lobby_sink;

static bool g_broker_connection = false;

// ongoing requests
static bool g_list_clients = false;
static bool g_list_servers = false;

static int _whisper(const char *dest, const char *msg)
{
    return BROKER_SEND(g_lobby_sink, BROKER_WHISP_FORMAT(g_name, dest, msg));
}

static int _send_to_lobby(const char *msg)
{
    return BROKER_SEND(g_lobby_sink, BROKER_MSG_FORMAT(g_name, msg));
}

static int _list_clients()
{
    return BROKER_SEND(g_lobby_sink, BROKER_LIST_FORMAT(g_name, BROKER_LIST_CLIENTS));
}

static int _list_servers()
{
    return BROKER_SEND(g_lobby_sink, BROKER_LIST_FORMAT(g_name, BROKER_LIST_SERVERS));
}

static int _ping()
{
    return BROKER_SEND(g_lobby_sink, BROKER_PING_FORMAT(g_name, BROKER_PING_CLIENT));
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

static void _parse_user_input(const char *input_buffer)
{
    if (input_buffer[0] == '/') {
        if (strncmp(input_buffer, "/help", strlen("/help")) == 0) {
            printf("  /help           Print this help message\n");
            printf("  /clients        Show the clients currently in this lobby\n");
            printf("  /servers        Show the servers availables\n");
            printf("  /msg <u> <m>    Send the message <m> to client <u>\n");
        } else if (strncmp(input_buffer, "/clients", strlen("/clients")) == 0) {
            _list_clients();
            g_list_clients = true;
        } else if (strncmp(input_buffer, "/servers", strlen("/servers")) == 0) {
            _list_servers();
            g_list_servers = true;
        } else if (strncmp(input_buffer, "/msg", strlen("/msg")) == 0) {
            char whisp_buffer[BROKER_MAX_MSG_LENGTH];
            strcpy(whisp_buffer, input_buffer);

            // discard the command token
            (void)strtok(whisp_buffer, " ");

            char *user = strtok(NULL, " ");
            if (user == NULL) {
                fprintf(stderr, "*** You must provide a client name to send a message to.\n");
                return;
            }

            char *msg = strtok(NULL, " ");
            if (msg == NULL) {
                fprintf(stderr, "*** You must provide a message to send to '%s'.\n", user);
                return;
            }

            _whisper(user, msg);
        }
    } else {
        _send_to_lobby(input_buffer);
    }
}

static void _print_clients(char *list)
{
    char *name = strtok(list, BROKER_UNIT_SEPARATOR);
    while (name) {
        printf("  c: %s\n", name);
        name = strtok(NULL, BROKER_UNIT_SEPARATOR);
    }
}

static void _print_servers(char *list)
{
    char *name = strtok(list, BROKER_UNIT_SEPARATOR);
    while (name) {
        printf("  s: %s\n", name);
        name = strtok(NULL, BROKER_UNIT_SEPARATOR);
    }
}

static void _read_from_lobby()
{
    char *data = NULL;
    int bytes = nn_recv(g_lobby_sub, &data, NN_MSG, 0);
    assert(bytes >= 0);

    g_broker_connection = true;

    /*if (strncmp(strstr(data, BROKER_RECORD_SEPARATOR) + strlen(BROKER_RECORD_SEPARATOR), BROKER_PING, 4) != 0) printf("'%s'\n", data);*/

    char *user = BROKER_FIRST_TOKEN(data);
    char *cmd = BROKER_NEXT_TOKEN();

    if (strcmp(cmd, BROKER_PING) == 0) {
        // ignore
    } else if (strcmp(cmd, BROKER_MSG) == 0) {
        char *msg = BROKER_NEXT_TOKEN();

        printf("<lobby> %s: %s\n", user, msg);
    } else if (strcmp(cmd, BROKER_WHISP) == 0) {
        char *dest = BROKER_NEXT_TOKEN();
        char *msg = BROKER_NEXT_TOKEN();

        if (strcmp(dest, g_name) == 0) {
            printf("<whisp> %s: %s\n", user, msg);
        }
    } else if (strcmp(cmd, BROKER_INFO) == 0) {
        char *info_type = BROKER_NEXT_TOKEN();

        if (g_list_clients && strcmp(info_type, BROKER_INFO_CLIENTS) == 0) {
            char *list = BROKER_NEXT_TOKEN();
            _print_clients(list);
            g_list_clients = false;
        } else if (g_list_servers && strcmp(info_type, BROKER_INFO_SERVERS) == 0) {
            char *list = BROKER_NEXT_TOKEN();
            _print_servers(list);
            g_list_servers = false;
        }
    } else if (strcmp(cmd, BROKER_SHUTDOWN) == 0) {
        g_broker_connection = false;
    }

    nn_freemsg(data);
}

static void _input_enable()
{
}

static void _input_disable()
{
}

static int _poll()
{
    struct pollfd nodes[3];

    // stdin
    nodes[0].events = POLLIN;

    // broker lobby
    size_t fd_size = sizeof(nodes[1].fd);
    assert(nn_getsockopt(g_lobby_sub, NN_SOL_SOCKET, NN_RCVFD, &nodes[1].fd, &fd_size) == 0);
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
    bool connected = false;
    for (;;) {
        if (connected && !g_broker_connection) {
            printf("[C] --- Lost connection to lobby, reconnecting ...\n");
            _input_disable();
            // disable polling on input
            nodes[0].fd = 0;
            // disable hearthbeat timer
            nodes[2].fd = 0;
        } else if (!connected && g_broker_connection) {
            static bool print_once = true;
            printf("[C] +++ Connected to lobby !\n");
            if (print_once) {
                printf("Type '/help' for a list of commands.\n");
                print_once = false;
            }
            _input_enable();
            // enable polling on input
            nodes[0].fd = STDIN_FILENO;
            // enable hearthbeat timer
            nodes[2].fd = tfd;
            assert(timerfd_settime(nodes[2].fd, 0, &ts, NULL) == 0);
        }

        connected = g_broker_connection;

        int rc = poll(nodes, sizeof(nodes) / sizeof(struct pollfd), -1);
        if (rc == -1) {
            fprintf(stderr, "[C] poll() error: %s\n", strerror(errno));

            return -1;
        } else if (rc == 0) {
            // timeout
            continue;
        }

        if (nodes[0].revents & POLLIN) {
            memset(input_buffer, 0, sizeof(input_buffer));
            if (_get_user_input(input_buffer, sizeof(input_buffer)) == 0) {
                _parse_user_input(input_buffer);
            }
        }

        if (nodes[1].revents & POLLIN) {
            _read_from_lobby();
        }

        if (nodes[2].revents & POLLIN) {
            uint64_t res;
            int rc = read(nodes[2].fd, &res, sizeof(res));
            if (rc == -1 && errno != EAGAIN) {
                fprintf(stderr, "[C] read() error on timerfd: %s\n", strerror(errno));
            } else {
                _ping();
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "[C] Usage: ./%s <name>\n", argv[0]);
        return -1;
    }

    g_name = argv[1];
    g_name_len = strlen(g_name);

    printf("[C] --- Connecting as '%s' ...\n", g_name);

    g_lobby_sub = nn_socket(AF_SP, NN_SUB);
    assert(g_lobby_sub >= 0);
    assert(nn_setsockopt(g_lobby_sub, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
    assert(nn_connect(g_lobby_sub, BROKER_LOBBY) >= 0);

    g_lobby_sink = nn_socket(AF_SP, NN_PUSH);
    assert(g_lobby_sink >= 0);
    assert(nn_connect(g_lobby_sink, BROKER_SINK) >= 0);


    _poll();

    nn_shutdown(g_lobby_sink, 0);
    nn_shutdown(g_lobby_sub, 0);

    return 0;
}

