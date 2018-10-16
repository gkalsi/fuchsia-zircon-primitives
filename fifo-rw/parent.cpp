#define LOG_PREFIX "[PARNT]"
#include "common.h"

#include <string.h>

#include <fbl/auto_call.h>
#include <sys/types.h>
#include <zircon/syscalls.h>

zx_status_t fifo_recv(zx_handle_t fifo) {
    auto channel_cleanup = fbl::MakeAutoCall([fifo]() {
        zx_handle_close(fifo);
    });

    while (true) {
        // Pretend like this is a costly operation and slowly process items from
        // the fifo.
        zx_nanosleep(zx_deadline_after(ZX_MSEC(50)));

        zx_status_t st = zx_object_wait_one(
            fifo, ZX_FIFO_READABLE | ZX_FIFO_PEER_CLOSED,
            ZX_TIME_INFINITE, nullptr);

        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        uint64_t element;
        size_t actual;
        st = zx_fifo_read(fifo, kFifoMessageSize, &element, 1, &actual);
        if (st == ZX_ERR_PEER_CLOSED) {
            LOG("No more data to process, goodbye!\n");
            return ZX_OK;
        } else if (st != ZX_OK) {
            ERR("zx_fifo_read failed with st = %d\n", st);
            return st;
        }

        // Wait a little bit to prevent stdout interleaving.
        zx_nanosleep(zx_deadline_after(ZX_MSEC(50)));

        // Print what out peer sent us to stdout.
        LOG("Fifo Read Returned %lu\n", element);
    }

    return ZX_OK;
}

zx_status_t parent(zx_handle_t channel) {
    // Close the channel when we exit this scope.
    auto channel_cleanup = fbl::MakeAutoCall([channel]() {
        zx_handle_close(channel);
    });

    zx_handle_t mine, theirs;
    zx_status_t st = zx_fifo_create(kFifoDepth, kFifoMessageSize, 0,
                                    &mine, &theirs);
    if (st != ZX_OK) {
        ERR("zx_fifo_create failed with st = %d\n", st);
        return st;
    }

    zx_signals_t signals;
    st = zx_object_wait_one(channel,
                            ZX_CHANNEL_PEER_CLOSED | ZX_CHANNEL_WRITABLE,
                            ZX_TIME_INFINITE, &signals);

    if (st != ZX_OK) {
        ERR("zx_object_wait_one failed with st = %d\n", st);
        return st;
    }

    if (signals & ZX_CHANNEL_PEER_CLOSED) {
        ERR("Peer closed, quitting!\n");
        return ZX_ERR_PEER_CLOSED;
    }

    st = zx_channel_write(channel, 0, nullptr, 0, &theirs, 1);
    if (st != ZX_OK) {
        ERR("zx_channel_write failed with st = %d\n", st);
        return st;
    }

    return fifo_recv(mine);
}
