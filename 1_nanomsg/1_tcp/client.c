#include <stdio.h>
#include <assert.h>
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

int main(const int argc, const char **argv)
{
  int sock = nn_socket(AF_SP, NN_SUB);
  assert(sock >= 0);
  assert(nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "1", 1) >= 0);
  assert(nn_connect(sock, "ipc:///tmp/pubsub.ipc") >= 0);

  while (1) {
    char *buf = NULL;
    int bytes = nn_recv(sock, &buf, NN_MSG, 0);
    assert(bytes >= 0);
    printf("++ Client received '%s'\n", buf);
    nn_freemsg(buf);
  }

  return nn_shutdown(sock, 0);
}
