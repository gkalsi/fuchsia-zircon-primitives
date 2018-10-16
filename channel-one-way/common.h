#pragma once

#include <zircon/types.h>
#include <stdio.h>

zx_status_t parent(zx_handle_t channel);
zx_status_t child(zx_handle_t channel);

#define ERR(fmt, ...) fprintf(stderr, LOG_PREFIX " " fmt, ##__VA_ARGS__);
#define LOG(fmt, ...) fprintf(stdout, LOG_PREFIX " " fmt, ##__VA_ARGS__);

// Number of bytes in the message we're going to send over the channel.
constexpr size_t kMessageLength = 32;