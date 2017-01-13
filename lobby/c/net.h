#pragma once

#define NET_MAX_MSG_LENGTH                       512
#define NET_MAX_NAME_LENGTH                      64

#define NET_RECORD_SEPARATOR                     "\30"
#define NET_UNIT_SEPARATOR                       "\31"
#define NET_NEXT_TOKEN()                         strtok(NULL, NET_RECORD_SEPARATOR)
#define NET_FIRST_TOKEN(__s)                     strtok(__s, NET_RECORD_SEPARATOR)

#define NET_FORMAT_ARGS0(__u, __t)               "%s%s%s", __u, NET_RECORD_SEPARATOR, __t
#define NET_FORMAT_ARGS1(__u, __t, __a1)         "%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1
#define NET_FORMAT_ARGS2(__u, __t, __a1, __a2)   "%s%s%s%s%s%s%s", __u, NET_RECORD_SEPARATOR, __t, NET_RECORD_SEPARATOR, __a1, NET_RECORD_SEPARATOR, __a2

#define NET_SHUTDOWN                             "shutdown"

#define NET_MSG                                  "msg"

#define NET_WHISP                                "whisp"

#define NET_LIST                                 "list"
#define NET_LIST_CLIENTS                         "clients"
#define NET_LIST_SERVERS                         "servers"
#define NET_LIST_MAX_SIZE(__u, __t)              (NET_MAX_MSG_LENGTH - strlen(__u)  - strlen(NET_LIST) - strlen(__t) - (4 * strlen(NET_RECORD_SEPARATOR)))

#define NET_INFO                                 "info"
#define NET_INFO_CLIENTS                         "clients"
#define NET_INFO_SERVERS                         "servers"

#define NET_PING                                 "ping"
#define NET_PING_BROKER                          "broker"
#define NET_PING_CLIENT                          "client"
#define NET_PING_SERVER                          "server"

int net_whisper(int socket, const char *from, const char *to, const char *msg);
int net_msg(int socket, const char *from, const char *msg);
int net_list_clients(int socket, const char *from);
int net_list_servers(int socket, const char *from);
int net_info(int socket, const char *from, const char *info_type, const char *info);
int net_ping(int socket, const char *from, const char *type);
int net_shutdown(int socket, const char *from);

