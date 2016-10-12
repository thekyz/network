#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>

#include "list.h"
#include "dealer_game.h"

#define _CARD(__d, __did, __sid, __rid)                 \
        (__d)->cards[(__did) * (NSUITS * NRANKS) + (__sid) * NRANKS + (__rid)]

#define _COPY_CARD(__from, __to)                do {    \
        (__to).deck = (__from).deck;                    \
        (__to).suit = (__from).suit;                    \
        (__to).rank = (__from).rank;                    \
    } while (0)

static const char *const gsc_dealer_name = "dealer";

static const char *const gsc_ranks[] = {
    "ace", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "jack", "queen", "king"
 };

static const char *const gsc_suits[] = {
    "clubs", "diamonds", "hearts", "spades"
};


static void _init_deck(struct _deck *deck)
{
    int deck_it, rank_it, suit_it;
    for (deck_it = 0; deck_it < NDECKS ; deck_it++) {
        for (suit_it = 0; suit_it < NSUITS; suit_it++) {
            for (rank_it = 0; rank_it < NRANKS; rank_it++) {
                _CARD(deck, deck_it, suit_it, rank_it).deck = deck_it;
                _CARD(deck, deck_it, suit_it, rank_it).suit = suit_it;
                _CARD(deck, deck_it, suit_it, rank_it).rank = rank_it;
            }
        }
    }

    deck->hit_id = 0;

    int card_it;
    for (card_it = DECK_SIZE - 1;  card_it > 0; card_it--) {
        struct _card temp;
        int random_it = rand() % card_it;

        _COPY_CARD(deck->cards[card_it], temp);
        _COPY_CARD(deck->cards[random_it], deck->cards[card_it]);
        _COPY_CARD(temp, deck->cards[random_it]);
    }
}

static inline struct _card *_deal_one(struct _deck *deck)
{
    return &deck->cards[deck->hit_id++];
}

static struct _player *_get_player_from_name(struct _game *game, const char *name)
{
    struct _player *player = NULL;
    list_foreach(&game->players, player) {
        if (strncmp(player->name, name, MAX_PLAYER_NAME_LEN) == 0) {
            break;
        }
    }

    return player;
}

void game_init(struct _game *game)
{
    list_init(&game->players);
}

bool game_are_players_ready(struct _game *game)
{
    printf("[D] %d players connected ...\n", list_count(&game->players));
    struct _player *player;
    list_foreach(&game->players, player) {
        if (player->ready == false) {
            printf("[D] %s is not ready yet\n", player->name);
            return false;
        }
    }

    return true;
}

void game_start(struct _game *game)
{

}

void game_remove_player(struct _game *game, const char *name)
{
    struct _player *player = _get_player_from_name(game, name);
    if (player == NULL) {
        fprintf(stderr, "[D] Could not find player %s (delete) ...\n", name);
        return;
    }

    list_delete((list *)player);
    free(player);
}

void game_ready_player(struct _game *game, const char *name)
{
    struct _player *player = _get_player_from_name(game, name);
    if (player == NULL) {
        fprintf(stderr, "[D] Could not find player %s (ready) ...\n", name);
        return;
    }

    player->ready = true;
}

void game_add_player(struct _game *game, const char *name)
{
    struct _player *player = (struct _player *)calloc(1, sizeof(struct _player));
    if (player == NULL) {
        fprintf(stderr, "[D] Could not create player %s (%s) ...\n", name, strerror(errno));
        return;
    }

    snprintf(player->name, MAX_PLAYER_NAME_LEN - 1, "%s", name); 
    list_add_tail(&game->players, &player->node);
}

