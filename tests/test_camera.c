#include <stdio.h>

#include "render/camera.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[camera] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int near_float(float a, float b) {
    float delta = a - b;
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta < 0.001f;
}

int test_camera(void) {
    int failed = 0;
    GameCameraState camera = {0};
    failed += assert_true(game_camera_init(&camera, 800.0f, 600.0f) == GAME_CAMERA_RESULT_OK, "camera init");
    failed += assert_true(game_camera_set_zoom(&camera, 999.0f) == GAME_CAMERA_RESULT_OK, "zoom set");
    failed += assert_true(near_float(camera.zoom, camera.max_zoom), "zoom clamps high");
    failed += assert_true(game_camera_set_zoom(&camera, 32.0f) == GAME_CAMERA_RESULT_OK, "zoom reset");

    GameCameraPoint screen = {0};
    GameCameraPoint world = {2.0f, -1.0f};
    GameCameraPoint round_trip = {0};
    failed += assert_true(
        game_camera_world_to_screen(&camera, world, &screen) == GAME_CAMERA_RESULT_OK,
        "world to screen"
    );
    failed += assert_true(
        game_camera_screen_to_world(&camera, screen, &round_trip) == GAME_CAMERA_RESULT_OK,
        "screen to world"
    );
    failed += assert_true(near_float(world.q, round_trip.q) && near_float(world.r, round_trip.r), "round trip stable");

    GameHexAxial party_before = {10, 0};
    camera.center = (GameCameraPoint){30.0f, 0.0f};
    failed += assert_true(game_camera_apply_leash(&camera, party_before, 5.0f) == GAME_CAMERA_RESULT_OK, "apply leash");
    failed += assert_true(near_float(camera.center.q, 15.0f), "leash clamps camera center");
    failed += assert_true(party_before.q == 10 && party_before.r == 0, "leash does not mutate party position");

    if (failed == 0) {
        printf("[camera] PASS\n");
    }
    return failed;
}
