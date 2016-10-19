#pragma once

#include <time.h>

#include "list.h"

#define LOBBY_IN_PORT       "29001"
#define LOBBY_OUT_PORT      "29002"

#define MAX_MSG_LEN         256

#define A_CONNECTED         "#CONN#"
#define A_DISCONNECTED      "#DISC#"
#define A_READY             "#READY#"

#define IS_EQ(__a, __b)     (strncmp(__a, __b, strlen(__b)) == 0)

typedef void (*net_timer_cb)(void *ctx);
typedef void (*net_client_cb)(char *client, void *ctx);

struct _timer {
    list node;
    struct itimerspec value;
    int fd;
    net_timer_cb cb;
    void *ctx;
};

struct _net_cb {
    net_client_cb on_connect;
    net_client_cb on_disconnect;
    net_client_cb on_ready;
};

struct _net {
    void *zmq_ctx;
    void *lobby_in;
    void *lobby_out;
    list timers;
    bool timer_refresh;
};

int net_init();
int net_timeout(int timeout, net_timer_cb cb, void *ctx);
int net_periodic(int period, net_timer_cb cb, void *ctx);
int net_send(const char *filter, const char *msg);
int net_loop(struct _net_cb *net_cb, void *ctx);

