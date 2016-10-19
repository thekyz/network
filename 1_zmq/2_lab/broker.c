#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "net.h"

#define _ENLIGHT_COOKIE()  struct _context *ctx = (struct _context *)cookie

static void _player_disconnected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    printf("[D] %s disconnected from the lobby\n", player);
}

static void _player_ready(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    printf("[D] %s is ready\n", player);
}

static void _player_connected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    printf("[D] %s connected to the lobby\n", player);
}

int main(void)
{
    net_init();

    struct _net_cb *net_cb = (struct _net_cb *)malloc(sizeof(net_cb));
    net_cb->on_connect = _player_connected;
    net_cb->on_disconnect = _player_disconnected;
    net_cb->on_ready = _player_ready;

    net_loop(net_cb, NULL);

    free(net_cb);
}

