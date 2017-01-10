#pragma once

#include <stdbool.h>

#define CLIENT_NAME_MAX_LENGTH 64

struct client {
    char name[CLIENT_NAME_MAX_LENGTH];
    bool is_ready;
}

