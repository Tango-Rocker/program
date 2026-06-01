#pragma once

#include "core/log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_ASSERT(expr) do { \
    if (!(expr)) { \
        GAME_LOG_FATAL("Assertion failed: (%s)", #expr); \
    } \
} while (0)

#ifdef __cplusplus
}
#endif
