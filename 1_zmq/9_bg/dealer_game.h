#pragma once

#include <stdbool.h>

#include "list.h"

struct _card {
    int deck;
    int rank;
    int suit;
};

#define MAX_CARDS_IN_HAND 8

struct _hand {
    struct _card *cards[MAX_CARDS_IN_HAND];
    int ncards;
};

#define MAX_PLAYER_NAME_LEN 32

struct _player {
    list node;
    struct _hand hands[4];
    int nhands;    
    char name[MAX_PLAYER_NAME_LEN];
    bool ready;
    bool playing;
    int wager;
    int insurance;
};

#define NDECKS      1
#define NSUITS      4
#define NRANKS      13
#define DECK_SIZE   (NDECKS * NRANKS * NSUITS)

struct _deck {
    struct _card cards[DECK_SIZE];
    int hit_id;
};

struct _game {
    struct _deck deck;
    list players;
    int current_player_id;
};

void game_init(struct _game *game);

void game_init_deck();
char *game_card2str(struct _card *card, char *buffer);
char *game_score2str(struct _hand *hand, char *buffer);
void game_player_first_deal(const char *name);
int game_place_bet(const char *name, const char *bet);
bool game_are_bets_placed();
bool game_are_players_ready();
void game_remove_player(const char *name);
void game_ready_player(const char *name);
void game_ready_players_playing();
void game_add_player(const char *name);

