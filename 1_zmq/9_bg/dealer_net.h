#pragma once

#include <time.h>

#include "list.h"

#define DEALER_LOBBY_PORT   "29001"
#define DEALER_TABLE_PORT   "29002"

#define MAX_MSG_LEN         256

#define A_CONNECTED         "#CONN#"
#define A_DISCONNECTED      "#DISC#"
#define A_READY             "#READY#"
#define A_PLAY              "#PLAY#"

#define F_MSG               "#MSG#"
#define F_GAME              "#GAME#"

#define G_INIT              "+INIT+"
#define G_COUNTDOWN         "+COUNTDOWN+"
#define G_BET               "+BET+"

#define IS_EQ(__a, __b)     (strncmp(__a, __b, strlen(__b)) == 0)

typedef void (*net_timer_cb)(void *ctx);
typedef void (*net_client_cb)(char *client, void *ctx);
typedef void (*net_action_cb)(char *client, char *action, char *option, void *ctx);

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
    net_action_cb on_game_action;
};

int net_init(struct _net *net);
int net_timeout(struct _net *net, int timeout, net_timer_cb cb, void *ctx);
int net_periodic(struct _net *net, int period, net_timer_cb cb, void *ctx);
int net_send(struct _net *net, const char *filter, const char *msg);
int net_loop(struct _net *net, void *ctx);

