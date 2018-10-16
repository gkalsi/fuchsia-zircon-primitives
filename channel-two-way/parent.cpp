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
constexpr uint kMessageTimeoutMs = 500;

zx_status_t parent(zx_handle_t channel) {
    // Close the channel when we exit this scope.
    auto channel_cleanup = fbl::MakeAutoCall([channel]() {
        zx_handle_close(channel);
    });

    // Serve requests until the peer closes.
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

        // Note the nuance here: there could still be requests pending in the
        // channel. We're going to abandon all of those reqeusts.
        // If we wanted to drain the channel and read all the messages pending
        // inside we would omit the following block of code.
        if (signals & ZX_CHANNEL_PEER_CLOSED) {
            LOG("peer went away, shutting down\n");
            return ZX_OK;
        }

        add_request_t request;
        add_response_t response;
        st = zx_channel_read(
            channel,
            0, // Options
            &request,
            nullptr, // Handles
            sizeof(request),
            0,       // Num handles
            nullptr, // Actual bytes transferred.
            nullptr  // Actual handles transferred.
            );

        if (st != ZX_OK) {
            ERR("zx_channel_read failed with st = %d\n", st);
            return st;
        }

        response.result = request.a + request.b;
        LOG("child asked what is '%d + %d' respond with %d\n",
            request.a, request.b, response.result);

        st = zx_object_wait_one(channel,
                                ZX_CHANNEL_PEER_CLOSED | ZX_CHANNEL_WRITABLE,
                                ZX_TIME_INFINITE, &signals);
        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        if (signals & ZX_CHANNEL_PEER_CLOSED) {
            ERR("peer closed before response could be delivered\n");
            return ZX_ERR_PEER_CLOSED;
        }

        st = zx_channel_write(
            channel,
            0,
            &response, sizeof(response),
            nullptr, 0);

        if (st != ZX_OK) {
            ERR("zx_channel_write failed with st = %d\n", st);
            return st;
        }
    }

    // Channel will close automatically because of the fbl::AutoCall above.
    LOG("Closing channel...\n");

    return ZX_OK;
}
