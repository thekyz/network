#pragma once

#include <assert.h>
#include <stdbool.h>

#define __UID2(__a, __b)    __a ## __b
#define __UID1(__a, __b)    __UID2(__a, __b)
#define __UID               __UID1(__it__, __LINE__)

#define list_foreach_noit(__l,__lp)                                                                 \
    __typeof__(__lp) UID;                                                                           \
    for (__lp = (__l)->next; UID = __lp ? __lp->next : NULL, __lp && (__lp != (__l)); __lp = UID)

#define list_foreach(__l,__lp)                                                                      \
    __typeof__(__lp) UID;                                                                           \
    for (__lp = (__typeof__(__lp))((__l)->next); UID = __lp ? (__typeof__(__lp))(((list *)(__lp))->next) : NULL, __lp && ((list *)__lp != (__l)); __lp = UID)

typedef struct _list {
    struct _list *next;
    struct _list *prev;
} list;

static inline void list_init(list *l)
{
    l->next = l;
    l->prev = l;
}

static inline bool list_is_empty(list *l)
{
    return l->next == l;
}

static inline void list_insert(list *entry, list *before, list *after)
{
    before->next = entry;
    after->prev = entry;
    entry->next = after;
    entry->prev = before;
}

static inline void list_add_head(list *l, list *entry)
{
    list_insert(entry, l, l->next);
}

static inline void list_add_tail(list *l, list *entry)
{
    list_insert(entry, l->prev, l);
}

static inline bool list_is_tail(list *l, list *entry)
{
    if (entry->next == l) {
        return true;
    }

    return false;
}

static inline void list_delete(list *entry)
{
    if (entry->next && entry->prev) {
        entry->next->prev = entry->prev;
        entry->prev->next = entry->next;
    }

    entry->prev = NULL;
    entry->next = NULL;
}

static inline int list_count(list *l)
{
    assert(l);

    int count = 0;

    list *lp = NULL;
    list_foreach_noit(l, lp) {
        count++;
    }

    return count;
}

