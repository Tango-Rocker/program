#include "render/camera.h"

static float game_camera_clamp_float(float value, float min_value, float max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float game_camera_sqrt_float(float value) {
    if (value <= 0.0f) {
        return 0.0f;
    }
    float estimate = value > 1.0f ? value : 1.0f;
    for (int i = 0; i < 8; ++i) {
        estimate = 0.5f * (estimate + value / estimate);
    }
    return estimate;
}

GameCameraResult game_camera_init(GameCameraState *camera, float viewport_width, float viewport_height) {
    if (!camera || viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return GAME_CAMERA_RESULT_INVALID_ARGUMENT;
    }

    *camera = (GameCameraState){
        .center = {0.0f, 0.0f},
        .zoom = 32.0f,
        .min_zoom = 8.0f,
        .max_zoom = 128.0f,
        .viewport_width = viewport_width,
        .viewport_height = viewport_height,
        .leash_radius = 0.0f,
        .leash_target = {0, 0},
    };
    return GAME_CAMERA_RESULT_OK;
}

GameCameraResult game_camera_set_zoom(GameCameraState *camera, float zoom) {
    if (!camera || camera->min_zoom <= 0.0f || camera->max_zoom < camera->min_zoom) {
        return GAME_CAMERA_RESULT_INVALID_ARGUMENT;
    }

    camera->zoom = game_camera_clamp_float(zoom, camera->min_zoom, camera->max_zoom);
    return GAME_CAMERA_RESULT_OK;
}

GameCameraResult game_camera_set_viewport(GameCameraState *camera, float viewport_width, float viewport_height) {
    if (!camera || viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return GAME_CAMERA_RESULT_INVALID_ARGUMENT;
    }

    camera->viewport_width = viewport_width;
    camera->viewport_height = viewport_height;
    return GAME_CAMERA_RESULT_OK;
}

GameCameraResult game_camera_world_to_screen(
    const GameCameraState *camera,
    GameCameraPoint world,
    GameCameraPoint *out_screen
) {
    if (!camera || !out_screen || camera->zoom <= 0.0f) {
        return GAME_CAMERA_RESULT_INVALID_ARGUMENT;
    }

    out_screen->q = (world.q - camera->center.q) * camera->zoom + camera->viewport_width * 0.5f;
    out_screen->r = (world.r - camera->center.r) * camera->zoom + camera->viewport_height * 0.5f;
    return GAME_CAMERA_RESULT_OK;
}

GameCameraResult game_camera_screen_to_world(
    const GameCameraState *camera,
    GameCameraPoint screen,
    GameCameraPoint *out_world
) {
    if (!camera || !out_world || camera->zoom <= 0.0f) {
        return GAME_CAMERA_RESULT_INVALID_ARGUMENT;
    }

    out_world->q = (screen.q - camera->viewport_width * 0.5f) / camera->zoom + camera->center.q;
    out_world->r = (screen.r - camera->viewport_height * 0.5f) / camera->zoom + camera->center.r;
    return GAME_CAMERA_RESULT_OK;
}

GameCameraResult game_camera_apply_leash(GameCameraState *camera, GameHexAxial target, float leash_radius) {
    if (!camera || leash_radius < 0.0f) {
        return GAME_CAMERA_RESULT_INVALID_ARGUMENT;
    }

    camera->leash_target = target;
    camera->leash_radius = leash_radius;

    float target_q = (float)target.q;
    float target_r = (float)target.r;
    float delta_q = camera->center.q - target_q;
    float delta_r = camera->center.r - target_r;
    float distance = game_camera_sqrt_float(delta_q * delta_q + delta_r * delta_r);
    if (distance <= leash_radius || distance <= 0.00001f) {
        return GAME_CAMERA_RESULT_OK;
    }

    float scale = leash_radius / distance;
    camera->center.q = target_q + delta_q * scale;
    camera->center.r = target_r + delta_r * scale;
    return GAME_CAMERA_RESULT_OK;
}
