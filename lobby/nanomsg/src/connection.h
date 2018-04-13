#pragma once

#include "list.h"

#define CONNECTION_NAME_MAX_LENGTH  64

typedef void (*connection_msg_cb)(char *user, char *data);
typedef void (*connection_input_cb)(const char *input_buffer);
typedef void (*connection_cb)();

struct connection {
    int lobby_pubsub;
    int lobby_sink;
    bool lobby_connected;

    int secondary_socket;
    bool secondary_poll;
    bool secondary_connected;
    connection_cb on_secondary;
    char secondary_name[CONNECTION_NAME_MAX_LENGTH];

    char name[CONNECTION_NAME_MAX_LENGTH];

    connection_msg_cb on_ping;
    connection_msg_cb on_msg;
    connection_msg_cb on_info;
    connection_msg_cb on_whisp;
    connection_msg_cb on_shutdown;
    connection_input_cb on_input;
    connection_cb on_control;
    connection_cb ping;
};

void connection_cleanup(struct connection *conn);
void connection_poll(struct connection *conn, const char *broker_addr);
