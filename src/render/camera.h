#pragma once

#include <stdint.h>

#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_CAMERA_RESULT_OK = 0,
    GAME_CAMERA_RESULT_INVALID_ARGUMENT = 1,
} GameCameraResult;

typedef struct {
    float q;
    float r;
} GameCameraPoint;

typedef struct {
    GameCameraPoint center;
    float zoom;
    float min_zoom;
    float max_zoom;
    float viewport_width;
    float viewport_height;
    float leash_radius;
    GameHexAxial leash_target;
} GameCameraState;

GameCameraResult game_camera_init(GameCameraState *camera, float viewport_width, float viewport_height);
GameCameraResult game_camera_set_zoom(GameCameraState *camera, float zoom);
GameCameraResult game_camera_set_viewport(GameCameraState *camera, float viewport_width, float viewport_height);
GameCameraResult game_camera_world_to_screen(
    const GameCameraState *camera,
    GameCameraPoint world,
    GameCameraPoint *out_screen
);
GameCameraResult game_camera_screen_to_world(
    const GameCameraState *camera,
    GameCameraPoint screen,
    GameCameraPoint *out_world
);
GameCameraResult game_camera_apply_leash(GameCameraState *camera, GameHexAxial target, float leash_radius);

#ifdef __cplusplus
}
#endif
