#include "core/time.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <time.h>
#include <unistd.h>
#endif

#include <stdint.h>

double game_time_now_seconds(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static int init = 0;
    LARGE_INTEGER counter;

    if (!init) {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }

    if (!QueryPerformanceCounter(&counter)) {
        return 0.0;
    }

    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        return 0.0;
    }
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
#endif
}

void game_time_sleep_ms(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec req = { ms / 1000, (long)(ms % 1000) * 1000000L };
    struct timespec rem;
    while (nanosleep(&req, &rem) == -1) {
#ifdef EINTR
        req = rem;
#else
        return;
#endif
    }
#endif
}
