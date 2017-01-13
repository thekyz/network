#pragma once

#define BROKER_LOBBY_FILE                           "/tmp/broker_lobby.ipc"
#define BROKER_LOBBY                                "ipc://" BROKER_LOBBY_FILE
#define BROKER_SINK_FILE                            "/tmp/broker_sink.ipc"
#define BROKER_SINK                                 "ipc://" BROKER_SINK_FILE

#define BROKER_HEARTHBEAT_PERIOD                    1
#define BROKER_KEEP_ALIVE_PERIOD                    5

#define BROKER_NAME                                 "broker"

