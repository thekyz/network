#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

int main(void)
{
    void *ctx = zmq_ctx_new();
    void *responder = zmq_socket(ctx, ZMQ_REP);
    int rc = zmq_bind(responder, "tcp://*:5555");
    assert(rc == 0);

    for (;;) {
        char message[10];
        zmq_recv(responder, message, 10, 0);
        printf("[S] << %s\n", message);

        sleep(1);

        zmq_send(responder, "world", 5, 0);
    }

    return 0;
}
