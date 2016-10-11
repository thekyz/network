#pragma once

#include <time.h>

#include "list.h"

#define DEALER_LOBBY_PORT               "29001"
#define DEALER_TABLE_PORT               "29002"

#define DEALER_LOBBY_MSG_LEN            128

#define DEALER_LOBBY_PLAYER_CONNECTED       'c'
#define DEALER_LOBBY_PLAYER_DISCONNECTED    'd'
#define DEALER_LOBBY_PLAYER_READY           'r'

typedef void (*net_timer_cb)(void *ctx);
typedef void (*net_client_cb)(char *client, void *ctx);

struct _timer {
    list node;
    struct itimerspec value;
    int fd;
    net_timer_cb cb;
    void *ctx;
};

struct _net {
    void *zmq_ctx;
    void *table;
    void *lobby;
    list timers; 
    bool timer_refresh;
    net_client_cb on_connect;
    net_client_cb on_disconnect;
    net_client_cb on_ready;
};

int net_init(struct _net *net);
int net_timeout(struct _net *net, int timeout, net_timer_cb cb, void *ctx);
int net_periodic(struct _net *net, int period, net_timer_cb cb, void *ctx);
int net_loop(struct _net *net, void *ctx);

