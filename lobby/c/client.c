#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>

#include "broker.h"

static const int g_max_input_length = 256;      // Max number of chars read from the input

char *g_name;
int g_name_len = 0;
int g_lobby_sub;
int g_lobby_sink;

static int _whisper(const char *dest, const char *msg)
{
    return BROKER_SEND(g_lobby_sink, BROKER_WHISP_FORMAT(g_name, dest, msg));
}

static int _send_to_lobby(const char *msg)
{
    return BROKER_SEND(g_lobby_sink, BROKER_MSG_FORMAT(g_name, msg));
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

    return 0;
}

static void _parse_user_input(const char *input_buffer)
{
    if (input_buffer[0] == '/') {
        if (strncmp(input_buffer, "/help", strlen("/help")) == 0) {
            printf("  /help           Print this help message\n");
            printf("  /clients        Show the clients currently in this lobby\n");
            printf("  /msg <u> <m>    Send the message <m> to client <u>\n");
        } else if (strncmp(input_buffer, "/clients", strlen("/clients")) == 0) {
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

static void _read_from_lobby()
{
    char *data = NULL;
    int bytes = nn_recv(g_lobby_sub, &data, NN_MSG, 0);
    assert(bytes >= 0);

    /*printf("'%s'\n", data);*/

    char *user = strtok(data, BROKER_UNIT_SEPARATOR);
    char *cmd = strtok(NULL, BROKER_UNIT_SEPARATOR);

    if (strcmp(cmd, BROKER_MSG) == 0) {
        char *msg = strtok(NULL, BROKER_UNIT_SEPARATOR);

        if (strcmp(user, g_name) != 0) {
            printf("<lobby> %s: %s\n", user, msg);
        }
    } else if (strcmp(cmd, BROKER_WHISP) == 0) {
        char *dest = strtok(NULL, BROKER_UNIT_SEPARATOR);
        char *msg = strtok(NULL, BROKER_UNIT_SEPARATOR);

        if (strcmp(dest, g_name) == 0) {
            printf("<whisp> %s: %s\n", user, msg);
        }
    }

    nn_freemsg(data);
}

static int _poll()
{
    struct pollfd nodes[2];

    // stdin
    nodes[0].fd = STDIN_FILENO;
    nodes[0].events = POLLIN;

    // the broker lobby
    size_t fd_size = sizeof(nodes[1].fd);
    assert(nn_getsockopt(g_lobby_sub, NN_SOL_SOCKET, NN_RCVFD, &nodes[1].fd, &fd_size) == 0);
    nodes[1].events = POLLIN;

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
            if (_get_user_input(input_buffer, sizeof(input_buffer)) == 0) {
                _parse_user_input(input_buffer);
            }
        }

        if (nodes[1].revents & POLLIN) {
            _read_from_lobby();
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

    printf("[C] ### Connecting as '%s' ...\n", g_name);

    g_lobby_sub = nn_socket(AF_SP, NN_SUB);
    assert(g_lobby_sub >= 0);
    assert(nn_setsockopt(g_lobby_sub, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
    assert(nn_connect(g_lobby_sub, BROKER_LOBBY) >= 0);

    g_lobby_sink = nn_socket(AF_SP, NN_PUSH);
    assert(g_lobby_sink >= 0);
    assert(nn_connect(g_lobby_sink, BROKER_SINK) >= 0);

    printf("[C] +++ Connected to broker !\n");
    printf("Type '/help' for a list of commands.\n");

    _poll();

    nn_shutdown(g_lobby_sink, 0);
    nn_shutdown(g_lobby_sub, 0);

    return 0;
}

