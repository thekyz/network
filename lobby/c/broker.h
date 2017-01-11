#pragma once

#define BROKER_LOBBY                    "ipc:///tmp/broker_lobby.ipc"
#define BROKER_SINK                     "ipc:///tmp/broker_sink.ipc"

#define BROKER_MAX_MSG_LENGTH           256
#define BROKER_UNIT_SEPARATOR           "\u241F"
#define BROKER_FORMAT_STRING            "%s%s%s%s%s"

#define BROKER_MSG                      "msg"
#define BROKER_MSG_FORMAT(__u, __m)     BROKER_FORMAT_STRING, __u, BROKER_UNIT_SEPARATOR, BROKER_MSG, BROKER_UNIT_SEPARATOR, __m

#define BROKER_CONN                     "conn"
#define BROKER_CONN_FORMAT(__u)         BROKER_FORMAT_STRING, __u, BROKER_UNIT_SEPARATOR, BROKER_CONN, BROKER_UNIT_SEPARATOR, "-"

#define BROKER_DISC                     "disc"
#define BROKER_DISC_FORMAT(__u)         BROKER_FORMAT_STRING, __u, BROKER_UNIT_SEPARATOR, BROKER_DISC, BROKER_UNIT_SEPARATOR, "-"

