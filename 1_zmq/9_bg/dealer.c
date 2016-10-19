#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "dealer_net.h"
#include "dealer_game.h"

#define MAX_PHASE_LEN       32

struct _context {
    struct _net net;
    struct _game game;
    char phase[MAX_PHASE_LEN];
    int countdown;
};

#define _ENLIGHT_COOKIE()  struct _context *ctx = (struct _context *)cookie

static void _first_deal(void *cookie)
{
    _ENLIGHT_COOKIE();

    net_send(&ctx->net, F_MSG, "Dealing ...");

    char deal_msg[MAX_MSG_LEN] = "";
    struct _player *player = NULL;
    list_foreach(&ctx->game.players, player) {
        if (player->playing) {
            char card1[128], card2[128], score[128];
            game_player_first_deal(player->name);
            sprintf(deal_msg, "%s [%s] [%s] = [%s]", player->name,
                    game_card2str(player->hands[0].cards[0], card1),
                    game_card2str(player->hands[0].cards[1], card2),
                    game_score2str(&player->hands[0], score)
                );
            net_send(&ctx->net, F_MSG, deal_msg);
        }
    }
}

static void _place_bets(void *cookie)
{
    _ENLIGHT_COOKIE();

    sprintf(ctx->phase, G_BET);

    net_send(&ctx->net, F_GAME, G_BET);
    net_send(&ctx->net, F_MSG, "Please place your bets.");
}

static void _on_timeout_countdown(void *cookie)
{
    _ENLIGHT_COOKIE();

    if (ctx->countdown) {
        char msg[MAX_MSG_LEN] = "";
        sprintf(msg, "Game starts in %d seconds.", ctx->countdown);
        net_send(&ctx->net, F_MSG, msg);

        ctx->countdown--;
        net_timeout(&ctx->net, 1000, _on_timeout_countdown, cookie);
    } else {
        net_send(&ctx->net, F_MSG, "Game starts now !");

        game_ready_players_playing();
        game_init_deck();

        net_timeout(&ctx->net, 0, _place_bets, cookie);
    }
}

static void _player_action(char *player, char *action, char *option, void *cookie)
{
    _ENLIGHT_COOKIE();

    printf("[D] %s %s %s\n", player, action, option);

  #define IS_ACT(__m) IS_EQ(action, __m)
    if (IS_ACT(G_BET)) {
        if (!IS_EQ(ctx->phase, G_BET)) {
            fprintf(stderr, "[D] %s cannot bet in %s phase.\n", player, ctx->phase);
            return;
        }

        if (game_place_bet(player, option) == 0) {
            char msg[MAX_MSG_LEN] = "";
            sprintf(msg, "%s placed a bet of %s", player, option);
            net_send(&ctx->net, F_MSG, msg);

            if (game_are_bets_placed()) {
                net_timeout(&ctx->net, 0, _first_deal, cookie);
            }
        }
    }
}

static void _player_disconnected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    game_remove_player(player);

    printf("[D] %s disconnected from the lobby\n", player);
}

static void _player_ready(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    game_ready_player(player);

    printf("[D] %s is ready\n", player);

    if (game_are_players_ready() && IS_EQ(ctx->phase, G_INIT)) {
        sprintf(ctx->phase, G_COUNTDOWN);
        ctx->countdown = 5;
        net_timeout(&ctx->net, 1000, _on_timeout_countdown, cookie);
    }
}

static void _player_connected(char *player, void *cookie)
{
    _ENLIGHT_COOKIE();

    game_add_player(player);

    printf("[D] %s connected to the lobby\n", player);
}

int main(void)
{
    struct _context ctx;

    game_init(&ctx.game);
    net_init(&ctx.net);

    sprintf(ctx.phase, G_INIT);
    ctx.net.on_connect = _player_connected;
    ctx.net.on_disconnect = _player_disconnected;
    ctx.net.on_ready = _player_ready;
    ctx.net.on_game_action = _player_action;

    printf("[D] Waiting for players ...\n");

    net_loop(&ctx.net, &ctx);
}

