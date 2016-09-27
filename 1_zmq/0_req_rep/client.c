#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    void *ctx = zmq_ctx_new();

    printf("[C] Connecting to server ...\n");
    void *requester = zmq_socket(ctx, ZMQ_REQ);
    zmq_connect(requester, "tcp://localhost:5555");

    zmq_send(requester, "hello", 5, 0);

    char message[10];
    zmq_recv(requester, message, 10, 0);
    printf("[C] << %s\n", message);

    return 0;
}
