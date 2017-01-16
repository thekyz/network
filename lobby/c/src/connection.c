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

#include "net.h"
#include "broker.h"

static const int g_max_input_length = 256;      // Max number of chars read from the input

#define _SERVER_MODE_GAME	"game"
#define _SERVER_MODE_CHAT	"chat"

#define _CLIENT_MODE_IDLE	"idle"
#define _CLIENT_MODE_READY	"ready"

#define err(__m, ...)		fprintf(stderr, "[%c] Error: " __m "\n", g_log_prefix, ##__VA_ARGS__);
#define log(__m, ...)		printf("[%s] " __m "\n", g_name, ##__VA_ARGS__);

static char g_log_prefix = '?';
static bool g_server;
static char *g_conn_type = NULL;

// connection info
static char *g_name;
static int g_name_len = 0;
static char g_state[NET_MAX_NAME_LENGTH];
static char g_address[NET_MAX_NAME_LENGTH];

// sockets
static int g_lobby_sub;
static int g_lobby_sink;
static int g_server_sub;
static int g_server_sink;

static char g_broker_lobby_addr[NET_MAX_NAME_LENGTH];
static char g_broker_sink_addr[NET_MAX_NAME_LENGTH];

static bool g_broker_connection = false;
static bool g_server_connection = false;

// ongoing requests
static bool g_list_clients = false;
static bool g_list_servers = false;

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
            log("  /help           Print this help message");
            log("  /servers        Show the servers availables");
            log("  /ready		   Toggle ready mode (only in server rooms)")
            log("  /clients        Show the clients currently in the lobby (and connected server room)");
            log("  /msg <u> <m>    Send the message <m> to client <u>");
        } else if (strncmp(input_buffer, "/clients", strlen("/clients")) == 0) {
            net_list_clients(g_lobby_sink, g_name);
            g_list_clients = true;
        } else if (strncmp(input_buffer, "/servers", strlen("/servers")) == 0) {
            net_list_servers(g_lobby_sink, g_name);
            g_list_servers = true;
        } else if (strncmp(input_buffer, "/msg", strlen("/msg")) == 0) {
            char whisp_buffer[NET_MAX_MSG_LENGTH];
            strcpy(whisp_buffer, input_buffer);

            // discard the command token
            (void)strtok(whisp_buffer, " ");

            char *user = strtok(NULL, " ");
            if (user == NULL) {
                err("*** You must provide a client name to send a message to.");
                return;
            }

            char *msg = strtok(NULL, " ");
            if (msg == NULL) {
                err("*** You must provide a message to send to '%s'.", user);
                return;
            }

            net_whisper(g_lobby_sink, g_name, user, msg);
        } else if (strncmp(input_buffer, "/ready", strlen("/ready")) == 0) {
            if (g_server_connection) {
                if (strcmp(g_state, _CLIENT_MODE_IDLE) == 0) {
                    sprintf(g_state, "%s", _CLIENT_MODE_READY);
                } else {
                    sprintf(g_state, "%s", _CLIENT_MODE_IDLE);
                }

                // notify the broker of our state change
                net_ping(g_lobby_sink, g_name, g_conn_type, g_state, g_address);
            }
        }
    } else {
        net_msg(g_lobby_sink, g_name, input_buffer);
    }
}

static void _read_from_lobby()
{
    char *data = NULL;
    int bytes = nn_recv(g_lobby_sub, &data, NN_MSG, 0);
    assert(bytes >= 0);

    g_broker_connection = true;

    /*if (strncmp(strstr(data, BROKER_RECORD_SEPARATOR) + strlen(BROKER_RECORD_SEPARATOR), BROKER_PING, 4) != 0) printf("'%s'\n", data);*/

    char *user = NET_FIRST_TOKEN(data);
    char *cmd = NET_NEXT_TOKEN();

    if (strcmp(cmd, NET_PING) == 0) {
        // ignore
    } else if (strcmp(cmd, NET_MSG) == 0) {
		if (!g_server) {
	        char *msg = NET_NEXT_TOKEN();
	        printf("<lobby> %s: %s\n", user, msg);
		}
    } else if (strcmp(cmd, NET_WHISP) == 0) {
        char *dest = NET_NEXT_TOKEN();
        char *msg = NET_NEXT_TOKEN();

        if (strcmp(dest, g_name) == 0) {
            printf("<whisp> %s: %s\n", user, msg);
        }
    } else if (strcmp(cmd, NET_INFO) == 0) {
        char *info_type = NET_NEXT_TOKEN();

        if (g_list_clients && strcmp(info_type, NET_INFO_CLIENTS) == 0) {
            char *name = NET_NEXT_TOKEN();
			char *state = NET_NEXT_TOKEN();
			log("  c: %s <%s>", name, state);
        } else if (g_list_servers && strcmp(info_type, NET_INFO_SERVERS) == 0) {
            char *name = NET_NEXT_TOKEN();
			char *state = NET_NEXT_TOKEN();
			log("  s: %s <%s>", name, state);
        } else if (strcmp(info_type, NET_INFO_END) == 0) {
			char *end_type = NET_NEXT_TOKEN();
			char *state = NET_NEXT_TOKEN();
			if (strcmp(end_type, NET_INFO_SERVERS) == 0) {
				g_list_servers = false;
			} else if (strcmp(end_type, NET_INFO_CLIENTS) == 0) {
				g_list_clients = false;
			}
        }
    } else if (strcmp(cmd, NET_SHUTDOWN) == 0) {
        if (strcmp(user, BROKER_NAME) == 0) {
            g_broker_connection = false;
        } else if (g_server && strcmp(user, NET_SHUTDOWN_SERVERS) == 0) {
            // TODO flag for shutdown if empty for more than 30 seconds, shutdown on second message
        }
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
            log("--- Lost connection to lobby, reconnecting ...");
            _input_disable();
            // disable polling on input
            nodes[0].fd = 0;
            // disable hearthbeat timer
            nodes[2].fd = 0;
        } else if (!connected && g_broker_connection) {
            static bool print_once = true;
            log("+++ Connected to lobby !");
            if (!g_server) {
                if (print_once) {
                    log("Type '/help' for a list of commands.");
                    print_once = false;
                }

                _input_enable();
                // enable polling on input
                nodes[0].fd = STDIN_FILENO;
            }

            // enable hearthbeat timer
            nodes[2].fd = tfd;
            assert(timerfd_settime(nodes[2].fd, 0, &ts, NULL) == 0);
        }

        connected = g_broker_connection;

        int rc = poll(nodes, sizeof(nodes) / sizeof(struct pollfd), -1);
        if (rc == -1) {
            err("poll() error: %s", strerror(errno));

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
                err("read() error on timerfd: %s", strerror(errno));
            } else {
                net_ping(g_lobby_sink, g_name, g_conn_type, g_state, g_address);
            }
        }
    }
}

static void _usage(const char *app)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "   %s <%s> <broker_addr> <name> <serv_addr>\n", app, NET_PING_SERVER);
	fprintf(stderr, "   %s <%s> <broker_addr> <name>\n", app, NET_PING_CLIENT);
}

int main(int argc, char **argv)
{
    if (argc < 4 || (strcmp(argv[1], NET_PING_SERVER) != 0 && strcmp(argv[1], NET_PING_CLIENT) != 0)) {
		_usage(argv[0]);
        return -1;
    }

    g_name = argv[3];
    g_name_len = strlen(g_name);
	g_conn_type = argv[1];

	if (strcmp(argv[1], NET_PING_SERVER) == 0) {
		if (argc < 5) {
			_usage(argv[0]);
			return -1;
		}

		g_server = true;
		g_log_prefix = 'S';
		sprintf(g_state, _SERVER_MODE_CHAT);
		sprintf(g_address, "%s", argv[3]);
	} else {
		g_server = false;
		g_log_prefix = 'C';
		sprintf(g_state, _CLIENT_MODE_IDLE);
		sprintf(g_address, "-");
	}

	sprintf(g_broker_lobby_addr, "tcp://%s:%s", argv[2], BROKER_LOBBY_PORT);
	sprintf(g_broker_sink_addr, "tcp://%s:%s", argv[2], BROKER_SINK_PORT);

    log("--- Connecting to %s as %s '%s' ...", g_broker_lobby_addr, g_conn_type, g_name);

    g_lobby_sub = nn_socket(AF_SP, NN_SUB);
    assert(g_lobby_sub >= 0);
    assert(nn_setsockopt(g_lobby_sub, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
	assert(nn_connect(g_lobby_sub, g_broker_lobby_addr) >= 0);

    g_lobby_sink = nn_socket(AF_SP, NN_PUSH);
    assert(g_lobby_sink >= 0);
    assert(nn_connect(g_lobby_sink, g_broker_sink_addr) >= 0);

    _poll();

    nn_shutdown(g_lobby_sink, 0);
    nn_shutdown(g_lobby_sub, 0);

    return 0;
}

