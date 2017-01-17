#pragma once

#define NET_MAX_MSG_LENGTH                      512
#define NET_MAX_NAME_LENGTH                     64

#define NET_RECORD_SEPARATOR                    "\30"
#define NET_UNIT_SEPARATOR                      "\31"
#define NET_NEXT_TOKEN()                        strtok(NULL, NET_RECORD_SEPARATOR)
#define NET_FIRST_TOKEN(__s)                    strtok(__s, NET_RECORD_SEPARATOR)

#define NET_SHUTDOWN                            "shutdown"
#define NET_SHUTDOWN_SERVERS                    "servers"

#define NET_MSG                                 "msg"

#define NET_WHISP                               "whisp"

#define NET_LIST                                "list"
#define NET_LIST_CLIENTS                        "clients"
#define NET_LIST_SERVERS                        "servers"

#define NET_INFO                                "info"
#define NET_INFO_CLIENTS                        "clients"
#define NET_INFO_SERVERS                        "servers"
#define NET_INFO_END							"end"

#define NET_PING                                "ping"
#define NET_PING_BROKER                         "broker"
#define NET_PING_CLIENT                         "client"
#define NET_PING_SERVER                         "server"

int net_whisper(int socket, const char *from, const char *to, const char *msg);
int net_msg(int socket, const char *from, const char *msg);
int net_list_clients(int socket, const char *from);
int net_list_servers(int socket, const char *from);
int net_info(int socket, const char *from, const char *conn_type, const char *name, const char *state, const char *connections);
int net_ping(int socket, const char *from, const char *type, const char *state, const char *id, const char *connections);
int net_shutdown(int socket, const char *from);

