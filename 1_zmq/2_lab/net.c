#include <sys/timerfd.h>
#include <unistd.h>
#include <zmq.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "list.h"

static struct _net g_net;

int net_init()
{
    g_net.zmq_ctx = zmq_ctx_new();
    g_net.lobby_out = zmq_socket(g_net.zmq_ctx, ZMQ_PUB);
    if (zmq_bind(g_net.lobby_out, "tcp://*:" LOBBY_OUT_PORT) != 0) {
        return -1;
    }

    g_net.lobby_in = zmq_socket(g_net.zmq_ctx, ZMQ_PULL);
    if (zmq_bind(g_net.lobby_in, "tcp://*:" LOBBY_IN_PORT) != 0) {
        return -1;
    }

    list_init(&g_net.timers);
    g_net.timer_refresh = true;

    return 0;
}

static struct _timer *_get_timer_from_fd(int fd)
{
    struct _timer *timer_it;
    list_foreach(&g_net.timers, timer_it) {
        if (timer_it->fd ==fd) {
            return timer_it;
        }
    }

    return NULL;
}

static void _delete_timer(struct _timer *timer)
{
    timer->value.it_value.tv_sec = 0;
    timer->value.it_value.tv_nsec = 0;
    timer->value.it_interval.tv_sec = 0;
    timer->value.it_interval.tv_nsec = 0;

    (void)timerfd_settime(timer->fd, 0, &timer->value, 0);

    list_delete(&timer->node);

    close(timer->fd);
    free(timer);

    g_net.timer_refresh = true;
}

static void _add_timer(struct _timer *timer)
{
    list_add_tail(&g_net.timers, &timer->node);

    g_net.timer_refresh = true;
}

static int _create_timer(int ms, bool periodic, net_timer_cb cb, void *ctx)
{
    assert(cb);

    if (ms == 0 && periodic) {
        fprintf(stderr, "[D] Error: must have a timeout for periodic timers!\n");
        return -1;
    }

    struct _timer *timer = (struct _timer *)malloc(sizeof(struct _net));
    if (timer == NULL) {
        fprintf(stderr, "[D] Error while allocating memory for timer!\n");
        return -1;
    }

    timer->fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (timer->fd == -1) {
        free(timer);
        fprintf(stderr, "[D] Error while creating timerfd for timer!\n");
        return -1;
    }

    timer->value.it_value.tv_sec = ms / 1000;
    timer->value.it_value.tv_nsec = (ms % 1000) * 1000000;

    if (ms == 0) {
        timer->value.it_value.tv_nsec = 1;
    }

    if (periodic) {
        timer->value.it_interval.tv_sec = ms / 1000;
        timer->value.it_interval.tv_nsec = (ms % 1000) * 1000000;
    } else {
        timer->value.it_interval.tv_sec = 0;
        timer->value.it_interval.tv_nsec = 0;
    }

    /*printf("new timer (s=%d / nsec=%d)\n", timer->value.it_value.tv_sec, timer->value.it_value.tv_nsec);*/

    timer->cb = cb;
    timer->ctx = ctx;

    if (timerfd_settime(timer->fd, 0, &timer->value, 0) != 0) {
        close(timer->fd);
        free(timer);
        fprintf(stderr, "[D] Error while setting time for timer!\n");
        return -1;
    }

    _add_timer(timer);

    return 0;
}

int net_timeout(int timeout, net_timer_cb cb, void *ctx)
{
    return _create_timer(timeout, false, cb, ctx);
}

int net_periodic(int period, net_timer_cb cb, void *ctx)
{
    return _create_timer(period, true, cb, ctx);
}

static void _on_timer(struct _timer *timer, uint64_t nhits)
{
    assert(timer);

    /*printf("[D] Timer %d hit (%llu hits).\n", timer->fd, nhits);*/
    timer->cb(timer->ctx);

    if (timer->value.it_interval.tv_nsec == 0 && timer->value.it_interval.tv_sec == 0) {
        _delete_timer(timer);
    }
}

int net_loop(struct _net_cb *net_cb, void *ctx)
{
    assert(net_cb);

    zmq_pollitem_t *items = NULL;
    int nitems = 0;

    for (;;) {
        if (g_net.timer_refresh) {
            if (items) {
                free(items);
            }

            nitems = list_count(&g_net.timers) + 1;
            items = (struct zmq_pollitem_t *)malloc(sizeof(struct zmq_pollitem_t) * nitems);

            items[0] = { g_net.lobby_in, 0, ZMQ_POLLIN, 0 };

            struct _timer *timer_it;
            int item_it = 1;
            list_foreach(&g_net.timers, timer_it) {
                items[item_it] = { 0, timer_it->fd, ZMQ_POLLIN, 0 };
                item_it++;
            }

            g_net.timer_refresh = false;
        }

        assert(items);

        int poll_status = zmq_poll(items, nitems, -1);
        if (poll_status == 0) {
            continue;
        }

        if (poll_status == -1) {
            fprintf(stderr, "poll failed: %s\n", strerror(errno));
            continue;
        }

        if (items[0].revents & ZMQ_POLLIN) {
            char msg[MAX_MSG_LEN];
            memset(&msg, 0, sizeof(msg));
            int len = zmq_recv(g_net.lobby_in, msg, sizeof(msg), 0);
            msg[len] = '\0';

            /*printf("[D] lobby << '%s'\n", msg);*/

          #define IS_MSG(__m) strncmp(msg, __m, strlen(__m)) == 0
            if (IS_MSG(A_CONNECTED)) {
                net_cb->on_connect(&msg[strlen(A_CONNECTED)], ctx);
            } else if (IS_MSG(A_DISCONNECTED)) {
                net_cb->on_disconnect(&msg[strlen(A_DISCONNECTED)], ctx);
            } else if (IS_MSG(A_READY)) {
                net_cb->on_ready(&msg[strlen(A_READY)], ctx);
            }
        }

        int item_it;
        for (item_it = 1; item_it < nitems; item_it++) {
            if (items[item_it].revents & ZMQ_POLLIN) {
                struct _timer *timer_it = _get_timer_from_fd(items[item_it].fd);
                uint64_t elapsed;

                int read_bytes = read(timer_it->fd, &elapsed, sizeof(elapsed));
                if (read_bytes < 0) {
                    fprintf(stderr, "Failed to read timer %d : %s\n", timer_it->fd, strerror(errno));
                    continue;
                }

                _on_timer(timer_it, elapsed);
            }
        }
    }
}

