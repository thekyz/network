#pragma once

#define BROKER_LOBBY                                "ipc:///tmp/broker_lobby.ipc"
#define BROKER_SINK                                 "ipc:///tmp/broker_sink.ipc"

#define BROKER_MAX_MSG_LENGTH                       256
#define BROKER_UNIT_SEPARATOR                       "\u241F"
#define BROKER_FORMAT_ARGS0(__u, __t)               "%s%s%s", __u, BROKER_UNIT_SEPARATOR, __t
#define BROKER_FORMAT_ARGS1(__u, __t, __a1)         "%s%s%s%s%s", __u, BROKER_UNIT_SEPARATOR, __t, BROKER_UNIT_SEPARATOR, __a1
#define BROKER_FORMAT_ARGS2(__u, __t, __a1, __a2)   "%s%s%s%s%s%s%s", __u, BROKER_UNIT_SEPARATOR, __t, BROKER_UNIT_SEPARATOR, __a1, BROKER_UNIT_SEPARATOR, __a2

#define BROKER_MSG                                  "msg"
#define BROKER_MSG_FORMAT(__u, __m)                 BROKER_FORMAT_ARGS1(__u, BROKER_MSG, __m)

#define BROKER_WHISP                                "whisp"
#define BROKER_WHISP_FORMAT(__u, __d, __m)          BROKER_FORMAT_ARGS2(__u, BROKER_WHISP, __d, __m)

#define BROKER_CONN                                 "conn"
#define BROKER_CONN_FORMAT(__u)                     BROKER_FORMAT_ARGS0(__u, BROKER_CONN)

#define BROKER_DISC                                 "disc"
#define BROKER_DISC_FORMAT(__u)                     BROKER_FORMAT_ARGS0(__u, BROKER_DISC)

#define BROKER_SEND(__s, __f)      ({                                       \
        char buffer[BROKER_MAX_MSG_LENGTH];                                 \
        int buffer_size = snprintf(buffer, BROKER_MAX_MSG_LENGTH - 1, __f); \
        int bytes = nn_send(__s, buffer, buffer_size + 1, 0);               \
        assert(bytes == buffer_size + 1);                                   \
        bytes; })

