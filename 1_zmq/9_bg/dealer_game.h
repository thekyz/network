#pragma once

#include <stdbool.h>

struct _card {
    int deck;
    int rank;
    int suit;
};

struct _hand {
    struct _card cards[8];
    int ncards;
};

struct _player {
    struct _player *next;
    struct _player *prev;
    struct _hand hands[4];
    int nhands;    
    char name[256];
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
    struct _player *player_node;
    int current_player_id;
};

