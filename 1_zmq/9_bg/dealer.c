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
    int countdown;
};

#define _ENLIGHT_COOKIE()  struct _context *ctx = (struct _context *)cookie

static void _start_game(void *cookie)
{
}

static void _place_bets(void *cookie)
{
    _ENLIGHT_COOKIE();

    net_send(&ctx->net, DEALER_TABLE_FILTER_GAME, DEALER_TABLE_GAME_BET);
    net_send(&ctx->net, DEALER_TABLE_FILTER_MSG, "Please place your bets.");
    net_timeout(&ctx->net, 0, _start_game, cookie);
}

static void _on_timeout_countdown(void *cookie)
{
    _ENLIGHT_COOKIE();

    if (ctx->countdown) {
        char msg[DEALER_MAX_MSG_LEN] = "";
        sprintf(msg, "Game starts in %d seconds.", ctx->countdown);
        net_send(&ctx->net, DEALER_TABLE_FILTER_MSG, msg);

        ctx->countdown--;
        net_timeout(&ctx->net, 1000, _on_timeout_countdown, cookie);
    } else {
        net_send(&ctx->net, DEALER_TABLE_FILTER_MSG, "Game starts now !");

        net_timeout(&ctx->net, 0, _place_bets, cookie);
    }
}

static void _player_disconnected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    game_remove_player(&ctx->game, player);

    printf("[D] %s disconnected from the lobby\n", player);
}

static void _player_ready(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    game_ready_player(&ctx->game, player);

    printf("[D] %s is ready\n", player);

    if (game_are_players_ready(&ctx->game)) {
        ctx->countdown = 5;
        net_timeout(&ctx->net, 1000, _on_timeout_countdown, cookie);
    }
}

static void _player_connected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    game_add_player(&ctx->game, player);

    printf("[D] %s connected to the lobby\n", player);
}

int main(void)
{
    struct _context ctx;

    game_init(&ctx.game);
    net_init(&ctx.net);

    ctx.net.on_connect = _player_connected;
    ctx.net.on_disconnect = _player_disconnected;
    ctx.net.on_ready = _player_ready;

    printf("[D] Waiting for players ...\n");

    net_loop(&ctx.net, &ctx);
}

