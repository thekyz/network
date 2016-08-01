#include <stdio.h>
#include <assert.h>
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

int main(const int argc, const char **argv)
{
  int sock = nn_socket(AF_SP, NN_PUB);
  assert(sock >= 0);
  assert(nn_bind(sock, "ipc:///tmp/pubsub.ipc") >= 0);

  int i = 0;
  char buff[5] = "";
  while (1) {
    snprintf(buff, 5, "%d", i);
    int bytes = nn_send(sock, buff, 5, 0);
    i++;
    assert(bytes == 5);
    sleep(1);
  }

  return nn_shutdown(sock, 0);
}
