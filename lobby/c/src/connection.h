#pragma once

#include "list.h"
#include "net.h"

struct connection {
    list node;
    char name[NET_MAX_NAME_LENGTH];
    int alive;
	char state[NET_MAX_NAME_LENGTH];
	char id[NET_MAX_NAME_LENGTH];
    char connections[NET_MAX_NAME_LENGTH];
};

