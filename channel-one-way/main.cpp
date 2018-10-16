// This program creates a child process and hands it one end of a channel.
// Then it sends periodic messages to the child process which prints them out
// on stdout.

#include <stdio.h>
#include <string.h>

#include <lib/fdio/spawn.h>

#include <zircon/process.h>
#include <zircon/processargs.h>
#include <zircon/syscalls.h>
#include <zircon/types.h>

// Pass this as a command line argument to start as a child, otherwise start
// as a parent.
const char* kCmdLineChild = "child";


// Create a channel, spawn a child process and give it one end of the channel
// we just created.
zx_status_t spawn_child(const char* path, zx_handle_t* out) {
    zx_status_t st;

    const uint32_t kFlags = FDIO_SPAWN_CLONE_ALL;
    const char* kChildProcessArgs[] = { kCmdLineChild, nullptr };
    char error_bufffer[FDIO_SPAWN_ERR_MSG_MAX_LENGTH];


    // Create a channel.
    zx_handle_t mine = ZX_HANDLE_INVALID;
    zx_handle_t other = ZX_HANDLE_INVALID;
    st = zx_channel_create(0, &mine, &other);
    if (st != ZX_OK) {
        fprintf(stderr, "[PARENT]: Failed to create channel, st = %d\n", st);
        return st;
    }
    *out = mine;

    // This struct is a list of things that we're going to pass to the child
    // process.
    const fdio_spawn_action_t actions[] = {
        {.action = FDIO_SPAWN_ACTION_SET_NAME, .name = {.data = "child"}},
        {.action = FDIO_SPAWN_ACTION_ADD_HANDLE, .h = { .id = PA_USER0, .handle = other}},
    };

    // Start the child process.
    st = fdio_spawn_etc(
        ZX_HANDLE_INVALID,  // Job handle, invalid uses default job
        kFlags,
        path,
        kChildProcessArgs,  // child process arguments.
        nullptr,            // environment, not needed for us.
        countof(actions),
        actions,
        nullptr,            // process out handle, ignored for now.
        error_bufffer
    );

    if (st != ZX_OK) {
        fprintf(stderr, "could not spawn child process, st = %d\n", st);
        fprintf(stderr, "reason: %s\n", error_bufffer);
        return st;
    }

    return ZX_OK;
}

// Periodically send a message to the child process via the channel.
int parent(zx_handle_t channel) {
    char message[32] = "Hello World!";

    while (true) {
        // Write the message.
        zx_status_t st = zx_channel_write(channel, 0, message, 32, nullptr, 0);
        if (st != ZX_OK) {
            fprintf(stderr, "zx_channel_write failed with rc = %d\n", st);
            return -1;
        }

        // Wait for a second.
        zx_nanosleep(zx_deadline_after(ZX_SEC(1)));
    }

    return 0;
}

// Print any message that the parent sends us out to stdout.
int child(zx_handle_t channel) {
    while (true) {
        zx_status_t st = zx_object_wait_one(
            channel,
            ZX_SOCKET_READABLE | ZX_SOCKET_PEER_CLOSED,
            ZX_TIME_INFINITE, nullptr
        );

        if (st != ZX_OK) {
            fprintf(stderr, "zx_object_wait_one returned %d\n", st);
            return -1;
        }


        uint32_t actual_bytes = 0;
        uint32_t actual_handles = 0;
        char buffer[32];
        st = zx_channel_read(channel, 0, buffer, nullptr, 32, 0,
                             &actual_bytes, &actual_handles);

        if (st != ZX_OK) {
            fprintf(stderr, "zx_channel_read failed with st = %d\n", st);
            return -1;
        }

        printf("PARENT SAYS: %s\n", buffer);
    }

    return 0;
}

int main(int argc, const char* argv[]) {
    bool is_child = false;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(kCmdLineChild, argv[i])) is_child = true;
    }


    if (is_child) {
        zx_handle_t to_parent = zx_take_startup_handle(PA_HND(PA_USER0, 0));
        return child(to_parent);
    } else {
        zx_handle_t to_child;
        char path[64];
        snprintf(path, sizeof(path), "/boot/bin/%s", argv[0]);
        spawn_child(path, &to_child);
        return parent(to_child);
    }

    // Shouldn't get here.
    return -1;
}