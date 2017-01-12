#pragma once

#define BROKER_LOBBY_FILE                           "/tmp/broker_lobby.ipc"
#define BROKER_LOBBY                                "ipc://" BROKER_LOBBY_FILE
#define BROKER_SINK_FILE                            "/tmp/broker_sink.ipc"
#define BROKER_SINK                                 "ipc://" BROKER_SINK_FILE

#define BROKER_MAX_MSG_LENGTH                       512
#define BROKER_MAX_NAME_LENGTH                      64

#define BROKER_HEARTHBEAT_PERIOD                    1
#define BROKER_KEEP_ALIVE_PERIOD                    5

#define BROKER_RECORD_SEPARATOR                     "\30"
#define BROKER_UNIT_SEPARATOR                       "\31"
#define BROKER_NEXT_TOKEN()                         strtok(NULL, BROKER_RECORD_SEPARATOR)
#define BROKER_FIRST_TOKEN(__s)                     strtok(__s, BROKER_RECORD_SEPARATOR)

#define BROKER_FORMAT_ARGS0(__u, __t)               "%s%s%s", __u, BROKER_RECORD_SEPARATOR, __t
#define BROKER_FORMAT_ARGS1(__u, __t, __a1)         "%s%s%s%s%s", __u, BROKER_RECORD_SEPARATOR, __t, BROKER_RECORD_SEPARATOR, __a1
#define BROKER_FORMAT_ARGS2(__u, __t, __a1, __a2)   "%s%s%s%s%s%s%s", __u, BROKER_RECORD_SEPARATOR, __t, BROKER_RECORD_SEPARATOR, __a1, BROKER_RECORD_SEPARATOR, __a2

#define BROKER_NAME                                 "broker"

#define BROKER_SHUTDOWN                             "shutdown"
#define BROKER_SHUTDOWN_FORMAT(__u)                 BROKER_FORMAT_ARGS0(__u, BROKER_SHUTDOWN)

#define BROKER_MSG                                  "msg"
#define BROKER_MSG_FORMAT(__u, __m)                 BROKER_FORMAT_ARGS1(__u, BROKER_MSG, __m)

#define BROKER_WHISP                                "whisp"
#define BROKER_WHISP_FORMAT(__u, __d, __m)          BROKER_FORMAT_ARGS2(__u, BROKER_WHISP, __d, __m)

#define BROKER_LIST                                 "list"
#define BROKER_LIST_CLIENTS                         "clients"
#define BROKER_LIST_SERVERS                         "servers"
#define BROKER_LIST_MAX_SIZE(__u, __t)              (BROKER_MAX_MSG_LENGTH - strlen(__u)  - strlen(BROKER_LIST) - strlen(__t) - (4 * strlen(BROKER_RECORD_SEPARATOR)))
#define BROKER_LIST_FORMAT(__u, __t)                BROKER_FORMAT_ARGS1(__u, BROKER_LIST, __t)

#define BROKER_INFO                                 "info"
#define BROKER_INFO_CLIENTS                         "clients"
#define BROKER_INFO_SERVERS                         "servers"
#define BROKER_INFO_FORMAT(__u, __t, __i)           BROKER_FORMAT_ARGS2(__u, BROKER_INFO, __t, __i)

#define BROKER_PING                                 "ping"
#define BROKER_PING_BROKER                          "broker"
#define BROKER_PING_CLIENT                          "client"
#define BROKER_PING_SERVER                          "server"
#define BROKER_PING_FORMAT(__u, __t)                BROKER_FORMAT_ARGS1(__u, BROKER_PING, __t)

#define BROKER_SEND(__s, __f)      ({                                       \
        char buffer[BROKER_MAX_MSG_LENGTH];                                 \
        int buffer_size = snprintf(buffer, BROKER_MAX_MSG_LENGTH - 1, __f); \
        int bytes = nn_send(__s, buffer, buffer_size + 1, 0);               \
        assert(bytes == buffer_size + 1);                                   \
        bytes; })

