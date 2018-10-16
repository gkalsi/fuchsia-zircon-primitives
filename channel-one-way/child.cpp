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
        zx_signals_t signals;
        zx_status_t st = zx_object_wait_one(
            channel,
            ZX_CHANNEL_READABLE | ZX_CHANNEL_PEER_CLOSED,
            ZX_TIME_INFINITE, &signals);

        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        if (signals & ZX_CHANNEL_PEER_CLOSED) {
            LOG("Channel closed, goodbye!\n");
            return ZX_OK;
        }

        uint32_t actual_bytes = 0;
        uint32_t actual_handles = 0;
        char buffer[kMessageLength];
        memset(buffer, 0, kMessageLength);

        st = zx_channel_read(
            channel,
            0,                  // options
            buffer,             // buffer to read into
            nullptr,            // handles
            sizeof(buffer),     // bytes available in the buffer
            0,                  // number of handles
            &actual_bytes,      // actual number of bytes copied
            &actual_handles);

        if (st != ZX_OK) {
            ERR("zx_channel_read failed with st = %d\n", st);
            return st;
        }

        // Make sure buffer is null terminated then print it.
        buffer[kMessageLength - 1] = '\0';
        LOG("%s\n", buffer);
    }

    return 0;
}