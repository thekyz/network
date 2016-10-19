#include <sys/timerfd.h>
#include <unistd.h>
#include <zmq.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "dealer_net.h"
#include "list.h"

int net_init(struct _net *net)
{
    assert(net);

    net->zmq_ctx = zmq_ctx_new();
    net->table = zmq_socket(net->zmq_ctx, ZMQ_PUB);
    if (zmq_bind(net->table, "tcp://*:" DEALER_TABLE_PORT) != 0) {
        return -1;
    }

    net->lobby = zmq_socket(net->zmq_ctx, ZMQ_PULL);
    if (zmq_bind(net->lobby, "tcp://*:" DEALER_LOBBY_PORT) != 0) {
        return -1;
    }

    list_init(&net->timers);
    net->timer_refresh = true;

    return 0;
}

static struct _timer *_get_timer_from_fd(struct _net *net, int fd)
{
    struct _timer *timer_it;
    list_foreach(&net->timers, timer_it) {
        if (timer_it->fd ==fd) {
            return timer_it;
        }
    }

    return NULL;
}

static void _delete_timer(struct _net *net, struct _timer *timer)
{
    timer->value.it_value.tv_sec = 0;
    timer->value.it_value.tv_nsec = 0;
    timer->value.it_interval.tv_sec = 0;
    timer->value.it_interval.tv_nsec = 0;

    (void)timerfd_settime(timer->fd, 0, &timer->value, 0);

    list_delete(&timer->node);

    close(timer->fd);
    free(timer);

    net->timer_refresh = true;
}

static void _add_timer(struct _net *net, struct _timer *timer)
{
    list_add_tail(&net->timers, &timer->node);

    net->timer_refresh = true;
}

static int _create_timer(struct _net *net, int ms, bool periodic, net_timer_cb cb, void *ctx)
{
    assert(net);
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

    _add_timer(net, timer);

    return 0;
}

int net_timeout(struct _net *net, int timeout, net_timer_cb cb, void *ctx)
{
    return _create_timer(net, timeout, false, cb, ctx);
}

int net_periodic(struct _net *net, int period, net_timer_cb cb, void *ctx)
{
    return _create_timer(net, period, true, cb, ctx);
}

int net_send(struct _net *net, const char *filter, const char *msg)
{
    char buffer[MAX_MSG_LEN];

    if (strncmp(filter, F_MSG, strlen(F_MSG)) == 0) {
        printf("[D] msg >> %s\n", msg);
    }

    snprintf(buffer, MAX_MSG_LEN - 1, "%s%s", filter, msg);
    return zmq_send(net->table, buffer, strlen(buffer) + 1, 0);
}

static void _on_timer(struct _net *net, struct _timer *timer, uint64_t nhits)
{
    assert(net);
    assert(timer);

    /*printf("[D] Timer %d hit (%llu hits).\n", timer->fd, nhits);*/
    timer->cb(timer->ctx);

    if (timer->value.it_interval.tv_nsec == 0 && timer->value.it_interval.tv_sec == 0) {
        _delete_timer(net, timer);
    }
}

int net_loop(struct _net *net, void *ctx)
{
    assert(net);

    zmq_pollitem_t *items = NULL;
    int nitems = 0;

    for (;;) {
        if (net->timer_refresh) {
            if (items) {
                free(items);
            }

            nitems = list_count(&net->timers) + 1;
            items = (struct zmq_pollitem_t *)malloc(sizeof(struct zmq_pollitem_t) * nitems);

            items[0] = { net->lobby, 0, ZMQ_POLLIN, 0 };

            struct _timer *timer_it;
            int item_it = 1;
            list_foreach(&net->timers, timer_it) {
                items[item_it] = { 0, timer_it->fd, ZMQ_POLLIN, 0 };
                item_it++;
            }

            net->timer_refresh = false;
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
            int len = zmq_recv(net->lobby, msg, sizeof(msg), 0);
            msg[len] = '\0';

            /*printf("[D] lobby << '%s'\n", msg);*/

          #define IS_MSG(__m) strncmp(msg, __m, strlen(__m)) == 0
            if (IS_MSG(A_CONNECTED)) {
                net->on_connect(&msg[strlen(A_CONNECTED)], ctx);
            } else if (IS_MSG(A_DISCONNECTED)) {
                net->on_disconnect(&msg[strlen(A_DISCONNECTED)], ctx);
            } else if (IS_MSG(A_READY)) {
                net->on_ready(&msg[strlen(A_READY)], ctx);
            } else if (IS_MSG(A_PLAY)) {
                char *split = &msg[strlen(A_PLAY)];
                char *player = strsep(&split, " ");
                char *action = strsep(&split, " ");
                char *option = strsep(&split, " ");
                net->on_game_action(player, action, option, ctx);
            }
        }

        int item_it;
        for (item_it = 1; item_it < nitems; item_it++) {
            if (items[item_it].revents & ZMQ_POLLIN) {
                struct _timer *timer_it = _get_timer_from_fd(net, items[item_it].fd);
                uint64_t elapsed;

                int read_bytes = read(timer_it->fd, &elapsed, sizeof(elapsed));
                if (read_bytes < 0) {
                    fprintf(stderr, "Failed to read timer %d : %s\n", timer_it->fd, strerror(errno));
                    continue;
                }

                _on_timer(net, timer_it, elapsed);
            }
        }
    }
}

