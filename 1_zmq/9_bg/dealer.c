#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "dealer_net.h"
#include "dealer_game.h"

struct _context {
    struct _net net;
    struct _game game;
};

#define _ENLIGHT_COOKIE()  struct _context *ctx = (struct _context *)cookie

static void _player_disconnected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    //game_remove_player(&ctx->game, player);

    printf("[D] %s disconnected from the lobby\n", player);
}

static void _player_ready(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    //game_ready_player(&ctx->game, player);

    printf("[D] %s is ready\n", player);
}

static void _player_connected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    //game_add_player(&ctx->game, player);

    printf("[D] %s connected to the lobby\n", player);
}

static void _on_timeout(void *cookie) {
    printf("[D] timeout\n");
}

static void _on_period(void *cookie) {
    printf("[D] period\n");
}

int main(void)
{
    struct _context ctx;

    net_init(&ctx.net);
    ctx.net.on_connect = _player_connected;
    ctx.net.on_disconnect = _player_disconnected;
    ctx.net.on_ready = _player_ready;

    printf("[D] Waiting for players ...\n");

    net_timeout(&ctx.net, 5000, _on_timeout, NULL);
    net_periodic(&ctx.net, 2000, _on_period, NULL);

    net_loop(&ctx.net, &ctx);
}
