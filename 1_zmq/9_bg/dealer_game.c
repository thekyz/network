#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

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

