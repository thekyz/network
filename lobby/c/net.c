#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <nanomsg/nn.h>

#include "net.h"

#define _FORMAT_ARGS0(__u, __t)              		"%s%s%s", __u, NET_RECORD_SEPARATOR, __t
#define _FORMAT_ARGS1(__u, __t, __a1)        		"%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1
#define _FORMAT_ARGS2(__u, __t, __a1, __a2)  		"%s%s%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1, NET_RECORD_SEPARATOR, __a2
#define _FORMAT_ARGS3(__u, __t, __a1, __a2, __a3)  	"%s%s%s%s%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1, NET_RECORD_SEPARATOR, __a2, NET_RECORD_SEPARATOR, __a3

#define _SEND(__s, __f)      ({                                       \
        char buffer[NET_MAX_MSG_LENGTH];                                 \
        int buffer_size = snprintf(buffer, NET_MAX_MSG_LENGTH - 1, __f); \
        int bytes = nn_send(__s, buffer, buffer_size + 1, 0);               \
        assert(bytes == buffer_size + 1);                                   \
        bytes; })

inline int net_whisper(int socket, const char *from, const char *to, const char *msg)
{
    return _SEND(socket, _FORMAT_ARGS2(from, NET_WHISP, to, msg));
}

inline int net_msg(int socket, const char *from, const char *msg)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_MSG, msg));
}

inline int net_list_clients(int socket, const char *from)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_LIST, NET_LIST_CLIENTS));
}

inline int net_list_servers(int socket, const char *from)
{
    return _SEND(socket, _FORMAT_ARGS1(from, NET_LIST, NET_LIST_SERVERS));
}

inline int net_info(int socket, const char *from, const char *conn_type, const char *name, const char *state)
{
	return _SEND(socket, _FORMAT_ARGS3(from, NET_INFO, conn_type, name, state));
}

inline int net_ping(int socket, const char *from, const char *type, const char *state, const char *address)
{
    return _SEND(socket, _FORMAT_ARGS3(from, NET_PING, type, state, address));
}

inline int net_shutdown(int socket, const char *from)
{
	return _SEND(socket, _FORMAT_ARGS0(from, NET_SHUTDOWN));
}

