#define LOG_PREFIX "[CHILD]"
#include "common.h"

#include <string.h>

#include <fbl/auto_call.h>
#include <sys/types.h>
#include <zircon/syscalls.h>

constexpr uint kNumRequests = 10;

zx_status_t child(zx_handle_t channel) {
    // Close this end of the channel if we exit.
    auto channel_cleanup = fbl::MakeAutoCall([channel]() {
        zx_handle_close(channel);
    });

    for (uint i = 0; i < kNumRequests; i++) {
        add_request_t request = {.a = i, .b = i + 1};
        add_response_t response;

        zx_signals_t signals;
        zx_status_t st = zx_object_wait_one(
            channel, ZX_CHANNEL_PEER_CLOSED | ZX_CHANNEL_WRITABLE,
            ZX_TIME_INFINITE, &signals);

        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        if (signals & ZX_CHANNEL_PEER_CLOSED) {
            ERR("server closed channel unexpectedly\n");
            return ZX_ERR_PEER_CLOSED;
        }

        st = zx_channel_write(
            channel,
            0,
            &request, sizeof(request),
            nullptr, 0);

        if (st != ZX_OK) {
            ERR("zx_channel_write failed with st = %d\n", st);
            return st;
        }

        st = zx_object_wait_one(channel,
                                ZX_CHANNEL_PEER_CLOSED | ZX_CHANNEL_READABLE,
                                ZX_TIME_INFINITE, &signals);

        if (st != ZX_OK) {
            ERR("zx_object_wait_one failed with st = %d\n", st);
            return st;
        }

        uint32_t actual_bytes, actual_handles;
        st = zx_channel_read(
            channel,
            0,
            &response,
            nullptr,
            sizeof(response),
            0,
            &actual_bytes, &actual_handles);

        if (st != ZX_OK) {
            ERR("zx_channel_read failed with st = %d\n", st);
            return st;
        }

        LOG("%d + %d = %d\n", request.a, request.b, response.result);
    }

    LOG("All done, closing connection\n");

    return 0;
}