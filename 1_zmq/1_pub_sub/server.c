#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

const char *gs_suits[] = { "clubs", "diamonds", "hearts", "spades" };

int main(void)
{
    void *ctx = zmq_ctx_new();
    void *publisher = zmq_socket(ctx, ZMQ_PUB);
    int rc = zmq_bind(publisher, "tcp://*:5556");
    assert(rc == 0);

    printf("[S] Publishing ...\n");

    srand(time(NULL));
    for (;;) {
        int suit = rand() % 4;
        int rank = (rand() % 13) + 1;

        char message[32];
        snprintf(message, sizeof(message), "deal %d %s", rank, gs_suits[suit]);
        zmq_send(publisher, message, strlen(message), 0);

        sleep(1);
    }

    return 0;
}
