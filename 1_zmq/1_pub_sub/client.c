#include <zmq.h>
#include <zhelpers.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    void *ctx = zmq_ctx_new();

    printf("[C] Suscribing to server ...\n");
    void *subscriber = zmq_socket(ctx, ZMQ_SUB);
    zmq_connect(subscriber, "tcp://localhost:5556");

    const char *msg_filter = "deal";

    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, msg_filter, strlen(msg_filter));

    char msg[32];
    for (;;) {
        memset(&msg, 0, sizeof(msg));
        int len = zmq_recv(subscriber, msg, sizeof(msg), 0);
        msg[len] = '\0';

        char *split = msg;
        strsep(&split, " ");
        char *rank = strsep(&split, " ");
        char *suit = strsep(&split, " ");
        printf("[C] %s of %s\n", rank, suit);
    }

    return 0;
}
