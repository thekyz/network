#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <nanomsg/nn.h>

#include "net.h"

#define _MSG_FORMAT(__u, __m)                 NET_FORMAT_ARGS1(__u, NET_MSG, __m)
#define _WHISP_FORMAT(__u, __d, __m)          NET_FORMAT_ARGS2(__u, NET_WHISP, __d, __m)
#define _LIST_FORMAT(__u, __t)                NET_FORMAT_ARGS1(__u, NET_LIST, __t)
#define _INFO_FORMAT(__u, __t, __i)           NET_FORMAT_ARGS2(__u, NET_INFO, __t, __i)
#define _PING_FORMAT(__u, __t)                NET_FORMAT_ARGS1(__u, NET_PING, __t)
#define _SHUTDOWN_FORMAT(__u)                 NET_FORMAT_ARGS0(__u, NET_SHUTDOWN)

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

inline int net_info(int socket, const char *from, const char *info_type, const char *info)
{
	return _SEND(socket, _INFO_FORMAT(from, info_type, info));
}

inline int net_ping(int socket, const char *from, const char *type)
{
    return _SEND(socket, _PING_FORMAT(from, type));
}

inline int net_shutdown(int socket, const char *from)
{
	return _SEND(socket, _SHUTDOWN_FORMAT(from));
}

