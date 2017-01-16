#pragma once

#define BROKER_BASE_SERVER_PORT                     7000
#define BROKER_MAX_SPAWNED_SERVERS                  16

#define BROKER_LOBBY_PORT							"6990"
#define BROKER_LOBBY                                "tcp://*:" BROKER_LOBBY_PORT
#define BROKER_SINK_PORT							"6991"
#define BROKER_SINK                                 "tcp://*:" BROKER_SINK_PORT

#define BROKER_HEARTHBEAT_PERIOD                    1
#define BROKER_KEEP_ALIVE_PERIOD                    5
#define BROKER_SERVER_SHUTDOWN_CHECK_PERIOD         (BROKER_KEEP_ALIVE_PERIOD * 6)

#define BROKER_NAME                                 "broker"

