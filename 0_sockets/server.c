#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define SERVER_PORT     "6666"

#define MAX_CONNECTIONS 16

#define MSG_MAX_LEN     64

#define MSG_IN          "IN"
#define MSG_OUT         "OUT"
#define MSG_OK          "OK"
    
struct client {
  int socket;
  char nick[MSG_MAX_LEN];
  int is_in;
};

struct client clients_g[MAX_CONNECTIONS] = { 0 };

static int start_server()
{
  int server_socket;
  int status;
  struct addrinfo hints;
  struct addrinfo *server_info;
  int yes = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;        // Fill own IP automatically

  status = getaddrinfo(NULL, SERVER_PORT, &hints, &server_info);
  if  (status != 0) {
    fprintf(stderr, "--- getaddrinfo error: %s\n", gai_strerror(status));
    return 1;
  }

  server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
  if (server_socket < 0) {
    fprintf(stderr, "--- socket error: %s\n", strerror(errno));
    return -2;
  }

  status = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if (status < 0) {
    fprintf(stderr, "--- setsockopt error: %s\n", strerror(errno));
    return -3;
  }

  status = bind(server_socket, server_info->ai_addr, server_info->ai_addrlen);
  if (status < 0) {
    fprintf(stderr, "--- bind error: %s\n", strerror(errno));
    return -4;
  }

  freeaddrinfo(server_info);
  return server_socket;
}

struct client *client_in(int server_socket)
{
  int bytes;
  int it;
  int rc;
  char msg[MSG_MAX_LEN];
  struct client *client = NULL;
  int client_socket;
  struct sockaddr_storage client_address;
  socklen_t client_address_size;

  rc = listen(server_socket, MAX_CONNECTIONS);
  if (rc < 0) {
    rc = errno;
    fprintf(stderr, "--- listen error: %s\n", strerror(rc));
    return NULL;
  }

  client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);
  if (client_socket < 0) {
    rc = errno;
    fprintf(stderr, "--- accept error: %s\n", strerror(rc));
    return NULL;
  }

  for (it = 0; it < MAX_CONNECTIONS; it++) {
    if (clients_g[it].is_in == 0) {
      client = &clients_g[it];
    }
  }

  if (client == NULL) {
    fprintf(stderr, "--- All seats taken, refusing connection.\n");
    return NULL;
  }

  client->socket = client_socket;

  bytes = recv(client->socket, msg, sizeof(msg), 0);
  if (bytes < 0) {
    fprintf(stderr, "--- recv error: %s\n", strerror(errno));
    return NULL;
  }

  if ((strncmp(msg, MSG_IN, strlen(MSG_IN)) != 0) || (strlen(msg) < (strlen(MSG_IN + 2)))) {
    msg[bytes - 1] = '\0';
    fprintf(stderr, "--- Received unknown message: %s\n", msg);
    return NULL;
  }

  printf("+++ Incoming client: %s\n", msg);

  strcpy(client->nick, &msg[3]);
  sprintf(msg, "%s", MSG_OK);

  bytes = send(client->socket, msg, strlen(msg) + 1, 0);
  if (bytes < 0) {
    fprintf(stderr, "--- send error: %s\n", strerror(errno));
    return NULL;
  }

  client->is_in = 1;
  printf(">>> %s connected !\n", client->nick);

  return client;
}

int handle_msg(struct client *client)
{
  int bytes;
  char msg[MSG_MAX_LEN];

  bytes = recv(client->socket, msg, sizeof(msg), 0);
  if (bytes < 0) {
    fprintf(stderr, "--- recv error: %s\n", strerror(errno));
    close(client->socket);
    return -1;
  }

  fprintf(stderr, ">>> Received message: %s\n", msg);

  // OUT message
  if (strncmp(msg, MSG_OUT, strlen(MSG_OUT)) == 0) {
    sprintf(msg, "%s", MSG_OK);
    (void)send(client->socket, msg, strlen(msg) + 1, 0);
    // let's ignore the sent value here, we can't do a lot anyway ...
    close(client->socket);
    client->is_in = 0;
    printf("<<< %s disconnected (%s).\n", client->nick, msg);
    return 1;
  }

  // fallback case ...
  msg[bytes - 1] = '\0';
  fprintf(stderr, "--- Received unknown message: %s\n", msg);
  close(client->socket);
  return -1;
}

int server_loop(int server_socket)
{
  fd_set read_set;
  struct timeval timeout;
  int rc = 0;
  int it;
  int selected = 0;
  int select_max;

  FD_ZERO(&read_set);
  FD_SET(server_socket, &read_set);
  select_max = server_socket + 1;

  printf("+++ Listening on port '%s' ...\n", SERVER_PORT);

  while (1) {
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    printf(".\n");
    selected = select(select_max, &read_set, NULL, NULL, &timeout);
    if (selected < 0) {
      rc = errno;
      fprintf(stderr, "--- select error: %s\n", strerror(errno));
      return rc;
    } else if (selected == 0) {
      // timeout
      continue;
    }

    if (FD_ISSET(server_socket, &read_set)) {
      struct client *client = client_in(server_socket);
      if (client != NULL) {
        FD_SET(client->socket, &read_set);

        if (client->socket >= select_max) {
          select_max = client->socket + 1;
        }

        selected--;
      }
    }
    
    if (selected == 0) {
      continue;
    }

    for (it = 0; it < MAX_CONNECTIONS; it++) {
      if (clients_g[it].is_in == 0) {
        continue;
      }

      if (FD_ISSET(clients_g[it].socket, &read_set)) {
        rc = handle_msg(&clients_g[it]);
        if (rc != 0) {
          FD_CLR(clients_g[it].socket, &read_set);
          clients_g[it].socket = 0;
        }

        selected--;

        if (selected <= 0) {
          break;
        }
      }
    }

    if (selected) {
      fprintf(stderr, "--- Could not find selected socket ...\n");
    }
  };

  return rc;
}

int main(int argc, char *argv[])
{
  int server_socket;
  int rc = 0;

  server_socket = start_server();
  if (server_socket < 0) {
    return server_socket;
  }

  return server_loop(server_socket);
}

