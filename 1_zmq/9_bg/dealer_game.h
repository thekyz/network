#pragma once

#include <stdbool.h>

#include "list.h"

struct _card {
    int deck;
    int rank;
    int suit;
};

struct _hand {
    struct _card cards[8];
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
bool game_are_players_ready(struct _game *game);
void game_remove_player(struct _game *game, const char *name);
void game_ready_player(struct _game *game, const char *name);
void game_add_player(struct _game *game, const char *name);

