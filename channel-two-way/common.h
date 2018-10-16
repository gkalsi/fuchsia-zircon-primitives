#pragma once

#include <stdio.h>
#include <zircon/types.h>

zx_status_t parent(zx_handle_t channel);
zx_status_t child(zx_handle_t channel);

#define ERR(fmt, ...) fprintf(stderr, LOG_PREFIX " " fmt, ##__VA_ARGS__);
#define LOG(fmt, ...) fprintf(stdout, LOG_PREFIX " " fmt, ##__VA_ARGS__);

typedef struct add_request {
    uint32_t a;
    uint32_t b;
} add_request_t;

typedef struct add_response {
    uint32_t result;
} add_response_t;