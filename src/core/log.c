#include "core/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *game_log_level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

static GameLogLevel g_game_log_level = GAME_LOG_LEVEL_INFO;

void game_log_set_level(GameLogLevel level) {
    g_game_log_level = level;
}

GameLogLevel game_log_get_level(void) {
    return g_game_log_level;
}

void game_log(GameLogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level < g_game_log_level) {
        return;
    }

    FILE *stream = (level >= GAME_LOG_LEVEL_WARN) ? stderr : stdout;
    time_t now = time(NULL);
    struct tm tm_now;
    struct tm *tmp = localtime(&now);

    if (tmp) {
        tm_now = *tmp;
        char ts[32];
        if (strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now) > 0) {
            fprintf(stream, "%s [%s] %s:%d: ", ts, game_log_level_names[level], file, line);
        } else {
            fprintf(stream, "[%s] %s:%d: ", game_log_level_names[level], file, line);
        }
    } else {
        fprintf(stream, "[%s] %s:%d: ", game_log_level_names[level], file, line);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);
    fputc('\n', stream);
    fflush(stream);

    if (level == GAME_LOG_LEVEL_FATAL) {
        abort();
    }
}
