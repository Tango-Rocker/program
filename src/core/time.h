#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

double game_time_now_seconds(void);
void game_time_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
