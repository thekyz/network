#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <nanomsg/nn.h>

#include "net.h"

#define _FORMAT_ARGS0(__u, __t)              		"%s%s%s", __u, NET_RECORD_SEPARATOR, __t
#define _FORMAT_ARGS1(__u, __t, __a1)        		"%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1
#define _FORMAT_ARGS2(__u, __t, __a1, __a2)  		"%s%s%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1, NET_RECORD_SEPARATOR, __a2
#define _FORMAT_ARGS3(__u, __t, __a1, __a2, __a3)  	"%s%s%s%s%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1, NET_RECORD_SEPARATOR, __a2, NET_RECORD_SEPARATOR, __a3

#define _MSG_FORMAT(__u, __m)                 _FORMAT_ARGS1(__u, NET_MSG, __m)
#define _WHISP_FORMAT(__u, __d, __m)          _FORMAT_ARGS2(__u, NET_WHISP, __d, __m)
#define _LIST_FORMAT(__u, __t)                _FORMAT_ARGS1(__u, NET_LIST, __t)
#define _INFO_FORMAT(__u, __t, __n, __s)      _FORMAT_ARGS3(__u, NET_INFO, __t, __n, __s)
#define _PING_FORMAT(__u, __t, __s)           _FORMAT_ARGS2(__u, NET_PING, __t, __s)
#define _SHUTDOWN_FORMAT(__u)                 _FORMAT_ARGS0(__u, NET_SHUTDOWN)

#define _SEND(__s, __f)      ({                                       \
        char buffer[NET_MAX_MSG_LENGTH];                                 \
        int buffer_size = snprintf(buffer, NET_MAX_MSG_LENGTH - 1, __f); \
        int bytes = nn_send(__s, buffer, buffer_size + 1, 0);               \
        assert(bytes == buffer_size + 1);                                   \
        bytes; })

inline int net_whisper(int socket, const char *from, const char *to, const char *msg)
{
    return _SEND(socket, _WHISP_FORMAT(from, to, msg));
}

inline int net_msg(int socket, const char *from, const char *msg)
{
    return _SEND(socket, _MSG_FORMAT(from, msg));
}

inline int net_list_clients(int socket, const char *from)
{
    return _SEND(socket, _LIST_FORMAT(from, NET_LIST_CLIENTS));
}

inline int net_list_servers(int socket, const char *from)
{
    return _SEND(socket, _LIST_FORMAT(from, NET_LIST_SERVERS));
}

inline int net_info(int socket, const char *from, const char *conn_type, const char *name, const char *state)
{
	return _SEND(socket, _INFO_FORMAT(from, conn_type, name, state));
}

inline int net_ping(int socket, const char *from, const char *type, const char *state)
{
    return _SEND(socket, _PING_FORMAT(from, type, state));
}

inline int net_shutdown(int socket, const char *from)
{
	return _SEND(socket, _SHUTDOWN_FORMAT(from));
}

