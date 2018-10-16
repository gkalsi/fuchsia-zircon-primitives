#define LOG_PREFIX "[PARNT]"
#include "common.h"

#include <string.h>

#include <fbl/auto_call.h>
#include <sys/types.h>
#include <zircon/syscalls.h>

// Send this many messages before quitting.
constexpr uint kNumMessages = 10;

// Put some delays between actions so it's easier to follow the output of
// stdout. Also, since both processes share stdout, these delays prevent
// interleaving.
constexpr uint kMessageTimeoutMs = 100;

zx_status_t parent(zx_handle_t channel) {
    // Close the channel when we exit this scope.
    auto channel_cleanup = fbl::MakeAutoCall([channel]() {
        zx_handle_close(channel);
    });

    for (uint i = 0; i < kNumMessages; i++) {
        // Format a message for the client to print into this buffer.
        char message[kMessageLength];
        memset(message, 0, kMessageLength);
        snprintf(message, sizeof(message), "Hello World %d of %d!",
                 i + 1, kNumMessages);

        // Print a message to stdout telling the user what we're about to do.
        LOG("Sending Message '%s'\n", message);
        zx_nanosleep(zx_deadline_after(ZX_MSEC(kMessageTimeoutMs)));

        // Wait for the channel to become writeable or for the remote process
        // to close the handle.
        zx_signals_t signals;
        zx_status_t st = zx_object_wait_one(
            channel, ZX_CHANNEL_WRITABLE | ZX_CHANNEL_PEER_CLOSED,
            ZX_TIME_INFINITE, &signals);

        if (st != ZX_OK) {
            ERR("zx_object_wait one failed with st = %d\n", st);
            return st;
        }

        // Did the remote client process close the channel?
        // Alternatively, in this case we could ignore the signal and attempt
        // the zx_channel_write which would fail.
        if (signals & ZX_CHANNEL_PEER_CLOSED) {
            ERR("peer closed unexpectedly\n");
            return ZX_CHANNEL_PEER_CLOSED;
        }

        // Write our message into the channel.
        st = zx_channel_write(
            channel,
            0,                        // Options, must be zero for now.
            message, sizeof(message), // Buffer of data to send
            nullptr, 0                // Don't transfer any handles
            );

        if (st != ZX_OK) {
            ERR("zx_channel_write failed with st = %d\n", st);
            return st;
        }

        zx_nanosleep(zx_deadline_after(ZX_MSEC(kMessageTimeoutMs)));
    }

    // Channel will close automatically because of the fbl::AutoCall above.
    LOG("Closing channel...\n");
    zx_nanosleep(zx_deadline_after(ZX_MSEC(500)));

    return ZX_OK;
}
