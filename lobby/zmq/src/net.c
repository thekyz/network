#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include <zmq.h>

#include "net.h"

void net_hearthbeat(list *net_clients, const char *client_name, const char *client_state, const char *client_id, const char *client_connections, net_cb on_connect)
{
    struct net_client *net_client;
    list_foreach(net_clients, net_client) {
        if (strcmp(net_client->name, client_name) == 0) {
            // found it: keep alive and update state
            net_client->alive = 1;
            sprintf(net_client->state, "%s", client_state);
            sprintf(net_client->connections, "%s", client_connections);
            return;
        }
    }

    net_client = (struct net_client *)malloc(sizeof(struct net_client));
    sprintf(net_client->name, "%s", client_name);
    sprintf(net_client->state, "%s", client_state);
    sprintf(net_client->id, "%s", client_id);
    sprintf(net_client->connections, "%s", client_connections);
    net_client->alive = 1;
    list_add_tail(net_clients, &net_client->node);

    if (on_connect) {
        on_connect(net_client);
    }
}

void net_check_connections(list *net_clients, net_cb on_disconnect)
{
    struct net_client *net_client;
    list_foreach(net_clients, net_client) {
        if (net_client->alive == 0) {
            if (on_disconnect) {
                on_disconnect(net_client);
            }

            list_delete(&net_client->node);
            free(net_client);
        }

        net_client->alive = 0;
    }
}

#define NRS                                             NET_RECORD_SEPARATOR

#define _FORMAT_ARGS0(__u, __t)                         "%s%s%s", __u, NRS,  __t
#define _FORMAT_ARGS1(__u, __t, __a1)                   "%s%s%s%s%s", __u, NRS, __t, NRS, __a1
#define _FORMAT_ARGS2(__u, __t, __a1, __a2)             "%s%s%s%s%s%s%s", __u, NRS, __t, NRS, __a1, NRS, __a2
#define _FORMAT_ARGS3(__u, __t, __a1, __a2, __a3)       "%s%s%s%s%s%s%s%s%s", __u, NRS, __t, NRS, __a1, NRS, __a2, NRS, __a3
#define _FORMAT_ARGS4(__u, __t, __a1, __a2, __a3, __a4) "%s%s%s%s%s%s%s%s%s%s%s", __u, NRS, __t, NRS, __a1, NRS, __a2, NRS, __a3, NRS, __a4

#define _SEND(__s, __f)      ({                                             \
        char buffer[NET_MAX_MSG_LENGTH];                                    \
        int buffer_size = snprintf(buffer, NET_MAX_MSG_LENGTH - 1, __f);    \
        int bytes = zmq_send(__s, buffer, buffer_size + 1, 0);              \
        assert(bytes == buffer_size + 1);                                   \
        bytes; })

int net_whisper(void *socket, const char *from, const char *to, const char *msg)
{
    return _SEND(socket, _FORMAT_ARGS2(from, NET_WHISP, to, msg));
}

int net_msg(void *socket, const char *from, const char *msg)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_MSG, msg));
}

int net_list_clients(void *socket, const char *from)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_LIST, NET_LIST_CLIENTS));
}

int net_list_servers(void *socket, const char *from)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_LIST, NET_LIST_SERVERS));
}

int net_info(void *socket, const char *from, const char *conn_type, const char *name, const char *state, const char *connections)
{
    return _SEND(socket, _FORMAT_ARGS4(from, NET_INFO, conn_type, name, state, connections));
}

int net_connect(void *socket, const char *from, const char *to)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_CONNECT, to));
}

int net_ping(void *socket, const char *from, const char *type, const char *state, const char *id, const char *connections)
{
    return _SEND(socket, _FORMAT_ARGS4(from, NET_PING, type, state, id, connections));
}

int net_shutdown(void *socket, const char *from)
{
    return _SEND(socket, _FORMAT_ARGS0(from, NET_SHUTDOWN));
}

