#define LOG_PREFIX "[CHILD]"
#include "common.h"

#include <string.h>

#include <fbl/auto_call.h>
#include <sys/types.h>
#include <zircon/syscalls.h>

constexpr uint kNumRequests = 32;

zx_status_t fifo_send(zx_handle_t fifo) {
    auto channel_cleanup = fbl::MakeAutoCall([fifo]() {
        zx_handle_close(fifo);
    });

    // Construct a buffer containing the fibonacci sequence.
    uint64_t fibo[kNumRequests];
    uint64_t* fibo_head = fibo;
    size_t requests_remaining = kNumRequests;
    fibo[0] = 1;
    fibo[1] = 1;
    for (uint i = 2; i < kNumRequests; i++) {
        fibo[i] = fibo[i - 1] + fibo[i - 2];
    }

    while (requests_remaining) {
        // Wait until we can write into the fifo.
        zx_signals_t signals;
        zx_status_t st = zx_object_wait_one(
            fifo, ZX_FIFO_WRITABLE | ZX_FIFO_PEER_CLOSED, ZX_TIME_INFINITE,
            &signals);

        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        if (signals & ZX_FIFO_PEER_CLOSED) {
            ERR("peer closed unexpectedly\n");
            return ZX_ERR_PEER_CLOSED;
        }

        // Write as many messages as possible into the fifo.
        size_t actual_count;
        st = zx_fifo_write(fifo, kFifoMessageSize, fibo_head,
                           requests_remaining, &actual_count);

        if (st != ZX_OK) {
            ERR("zx_fifo_write failed with st = %d\n", st);
            return st;
        }

        fibo_head += actual_count;
        requests_remaining -= actual_count;

        LOG("Wrote %lu %s, %lu remaining\n",
            actual_count, (actual_count == 1) ? "entry" : "entries",
            requests_remaining);
    }

    LOG("Closing fifo, goodbye!\n");

    return ZX_OK;
}

zx_status_t child(zx_handle_t channel) {
    // Close this end of the channel if we exit.
    auto channel_cleanup = fbl::MakeAutoCall([channel]() {
        zx_handle_close(channel);
    });

    // Wait until our channel to the other process is readable.
    zx_signals_t signals;
    zx_status_t st = zx_object_wait_one(
        channel, ZX_CHANNEL_PEER_CLOSED | ZX_CHANNEL_READABLE,
        ZX_TIME_INFINITE, &signals);

    if (st != ZX_OK) {
        ERR("zx_object_wait_one failed with st = %d\n", st);
        return st;
    }

    if (signals & ZX_CHANNEL_PEER_CLOSED) {
        ERR("peer closed before sending fifo handle\n");
        return ZX_ERR_PEER_CLOSED;
    }

    // The remote process should have sent us a handle to a fifo.
    // Read that handle out of the channel.
    zx_handle_t fifo;
    uint32_t actual_bytes, actual_handles;
    st = zx_channel_read(channel,
                         0,
                         nullptr, &fifo,
                         0, 1,
                         &actual_bytes, &actual_handles);

    if (st != ZX_OK) {
        ERR("zx_channel_read failed with st = %d\n", st);
        return st;
    }

    if (fifo == ZX_HANDLE_INVALID || actual_handles == 0) {
        ERR("failed to get fifo handle from remote process\n");
        return ZX_ERR_INTERNAL;
    }

    // Start sending messages over the fifo.
    return fifo_send(fifo);
}