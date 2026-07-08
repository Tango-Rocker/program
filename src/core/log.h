#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GameLogLevel {
    GAME_LOG_LEVEL_TRACE = 0,
    GAME_LOG_LEVEL_DEBUG = 1,
    GAME_LOG_LEVEL_INFO = 2,
    GAME_LOG_LEVEL_WARN = 3,
    GAME_LOG_LEVEL_ERROR = 4,
    GAME_LOG_LEVEL_FATAL = 5,
} GameLogLevel;

void game_log_set_level(GameLogLevel level);
GameLogLevel game_log_get_level(void);
void game_log(GameLogLevel level, const char *file, int line, const char *fmt, ...);

#define GAME_LOG_TRACE(...) game_log(GAME_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define GAME_LOG_DEBUG(...) game_log(GAME_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define GAME_LOG_INFO(...)  game_log(GAME_LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define GAME_LOG_WARN(...)  game_log(GAME_LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define GAME_LOG_ERROR(...) game_log(GAME_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define GAME_LOG_FATAL(...) game_log(GAME_LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
