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

static struct _game *game = NULL;

char *game_card2str(struct _card *card, char *buffer)
{
    sprintf(buffer, "%s of %s", gsc_ranks[card->rank], gsc_suits[card->suit]);
    return buffer;
}

char *game_score2str(struct _hand *hand, char *buffer)
{
    int score_offset = 0;
    int score = 0;
    bool has_ace = false;
    int i;
    for (i = 0; i < MAX_CARDS_IN_HAND; i++) {
        struct _card *card_it = hand->cards[i];

        if (card_it == NULL) {
            break;
        }

        /* Aces count for 1 or 11, no need to account for more than one though as 2 would give a score of more 22 at least */
        if (card_it->rank == 0) {
            has_ace = true;
        }

        if (card_it->rank >= 9) {
            score += 10;
        } else {
            score += card_it->rank + 1;
        }
        /*score_offset += sprintf(&buffer[score_offset], "{%d} ", card_it->rank);*/
    }

    /* default score */
    score_offset += sprintf(&buffer[score_offset], "%d", score);
    if (has_ace) {
        /* score for ace combination */
        score_offset += sprintf(&buffer[score_offset], " | %d", score + 10);

        if (score + 10 == 21 && i == 2)  {
            score_offset += sprintf(&buffer[score_offset], " BLACKJACK!");
        }
    }

    return buffer;
}

void game_init_deck()
{
    struct _deck *deck = &game->deck;

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

    srand(time(NULL));
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

static struct _player *_get_player_from_name(const char *name)
{
    struct _player *player = NULL;
    list_foreach(&game->players, player) {
        if (strncmp(player->name, name, MAX_PLAYER_NAME_LEN) == 0) {
            break;
        }
    }

    return player;
}

void game_player_first_deal(const char *name)
{
    struct _player *player = _get_player_from_name(name);
    assert(player);

    player->hands[0].cards[0] = _deal_one(&game->deck);
    player->hands[0].cards[1] = _deal_one(&game->deck);
}

void game_init(struct _game *g)
{
    game = g;
    list_init(&game->players);
}

int game_place_bet(const char *name, const char *bet)
{
    struct _player *player = _get_player_from_name(name);
    if (player == NULL) {
        fprintf(stderr, "[D] Could not place bet for unknown player %s.\n", name);
        return -1;
    }

    if (player->playing == false) {
        fprintf(stderr, "[D] Could not place bet for not playing player %s.\n", name);
        return -1;
    }

    player->wager = strtol(bet, NULL, 0);

    return 0;
}

bool game_are_bets_placed()
{
    struct _player *player;
    list_foreach(&game->players, player) {
        if (player->wager == 0) {
            printf("[D] %s has not bet yet\n", player->name);
            return false;
        }
    }

    return true;
}

void game_ready_players_playing()
{
    struct _player *player;
    list_foreach(&game->players, player) {
        if (player->ready) {
            printf("[D] %s is playing\n", player->name);
            player->playing = true;
            player->ready = false;
        }
    }
}

bool game_are_players_ready()
{
    /*printf("[D] %d players connected ...\n", list_count(&game->players));*/
    struct _player *player;
    list_foreach(&game->players, player) {
        if (player->ready == false) {
            printf("[D] %s is not ready yet\n", player->name);
            return false;
        }
    }

    return true;
}

void game_remove_player(const char *name)
{
    struct _player *player = _get_player_from_name(name);
    if (player == NULL) {
        fprintf(stderr, "[D] Could not find player %s (delete) ...\n", name);
        return;
    }

    list_delete((list *)player);
    free(player);
}

void game_ready_player(const char *name)
{
    struct _player *player = _get_player_from_name(name);
    if (player == NULL) {
        fprintf(stderr, "[D] Could not find player %s (ready) ...\n", name);
        return;
    }

    player->ready = true;
}

void game_add_player(const char *name)
{
    struct _player *player = (struct _player *)calloc(1, sizeof(struct _player));
    if (player == NULL) {
        fprintf(stderr, "[D] Could not create player %s (%s) ...\n", name, strerror(errno));
        return;
    }

    snprintf(player->name, MAX_PLAYER_NAME_LEN - 1, "%s", name); 
    list_add_tail(&game->players, &player->node);
}

