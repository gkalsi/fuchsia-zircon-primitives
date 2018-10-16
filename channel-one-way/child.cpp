#define LOG_PREFIX "[CHILD]"
#include "common.h"

#include <string.h>

#include <fbl/auto_call.h>
#include <zircon/syscalls.h>

zx_status_t child(zx_handle_t channel) {
    // Close this end of the channel if we exit.
    auto channel_cleanup = fbl::MakeAutoCall([channel]() {
        zx_handle_close(channel);
    });

    while (true) {
        zx_status_t st = zx_object_wait_one(
            channel,
            ZX_CHANNEL_READABLE | ZX_CHANNEL_PEER_CLOSED,
            ZX_TIME_INFINITE, nullptr);

        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        char buffer[kMessageLength];
        memset(buffer, 0, kMessageLength);

        st = zx_channel_read(
            channel,
            0,              // options
            buffer,         // buffer to read into
            nullptr,        // handles
            sizeof(buffer), // bytes available in the buffer
            0,              // number of handles
            nullptr,        // Actual bytes read, don't care
            nullptr         // Actual handles read, don't care
            );

        if (st == ZX_ERR_PEER_CLOSED) {
            // No more data to read, and the peer closed the connection.
            // Exit gracefully.
            LOG("Peer closed, goodbye!\n");
            return ZX_OK;
        } else if (st != ZX_OK) {
            ERR("zx_channel_read failed with st = %d\n", st);
            return st;
        }

        // Make sure buffer is null terminated then print it.
        buffer[kMessageLength - 1] = '\0';
        LOG("%s\n", buffer);
    }

    return ZX_OK;
}