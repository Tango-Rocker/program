#include <stdio.h>
#include <string.h>

#include "audio/audio_request.h"
#include "event/event.h"
#include "sim/noise_system.h"
#include "sim/projectile.h"
#include "sim/status_effect.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[audio_request] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_audio_request(void) {
    int failed = 0;

    GameAudioRequestQueue queue = {0};
    GameAudioRequest requests[2] = {0};
    failed += assert_true(game_audio_request_init(&queue, requests, 2u) == GAME_AUDIO_REQUEST_RESULT_OK, "audio queue init");

    GameEvent event = {0};
    GameNoiseEmittedPayload noise_payload = {.origin_q = 1, .origin_r = 2, .intensity = 5, .max_radius = 2, .attenuation = 1, .decay_ticks = 0};
    event.type = GAME_EVENT_TYPE_NOISE_EMITTED;
    event.payload_size = sizeof(noise_payload);
    event.tick = 1u;
    memcpy(event.payload.bytes, &noise_payload, sizeof(noise_payload));
    failed += assert_true(
        game_audio_request_project_from_event(&event, 11u, 7u, &queue) == GAME_AUDIO_REQUEST_RESULT_OK,
        "project from noise"
    );

    GameProjectileImpactPayload impact_payload = {.projectile_id = 1u, .source_index = 1u, .source_generation = 0u, .target_q = 3, .target_r = 4, .damage = 9, .ability_id = 1u};
    GameEvent impact_event = {0};
    impact_event.type = GAME_EVENT_TYPE_PROJECTILE_IMPACT;
    impact_event.payload_size = sizeof(impact_payload);
    memcpy(impact_event.payload.bytes, &impact_payload, sizeof(impact_payload));
    failed += assert_true(
        game_audio_request_project_from_event(&impact_event, 12u, 8u, &queue) == GAME_AUDIO_REQUEST_RESULT_OK,
        "project from projectile"
    );

    failed += assert_true(queue.head == 0u, "queue head unchanged before pop");
    failed += assert_true(game_audio_request_count(&queue) == 2u, "queue has two requests");
    GameAudioRequest first = {0};
    failed += assert_true(game_audio_request_pop(&queue, &first) == GAME_AUDIO_REQUEST_RESULT_OK, "pop first");
    failed += assert_true(first.sound_id == 1u, "first event maps noise to sound id 1");
    failed += assert_true(first.source_event_sequence == 7u, "first request keeps source sequence");
    failed += assert_true(first.priority == 3u, "first request priority from noise");
    failed += assert_true(first.origin_q == noise_payload.origin_q && first.origin_r == noise_payload.origin_r, "origin copied from noise");

    GameAudioRequest second = {0};
    failed += assert_true(game_audio_request_pop(&queue, &second) == GAME_AUDIO_REQUEST_RESULT_OK, "pop second");
    failed += assert_true(second.sound_id == 2u, "second event maps impact to sound id 2");
    failed += assert_true(second.source_event_sequence == 8u, "second request keeps source sequence");

    failed += assert_true(
        game_audio_request_project_from_event(&(GameEvent){.type = GAME_EVENT_TYPE_STATUS_EFFECT_EXPIRED, .payload_size = 0u},
                                             13u,
                                             9u,
                                             &queue)
            == GAME_AUDIO_REQUEST_RESULT_BAD_PAYLOAD,
        "bad payload rejected"
    );

    failed += assert_true(
        game_audio_request_project_from_event(
            &(GameEvent){.type = GAME_EVENT_TYPE_THREAT_STATE_CHANGED, .payload_size = 0u},
            14u,
            9u,
            &queue
        ) == GAME_AUDIO_REQUEST_RESULT_OK,
        "threat state change projectable"
    );
    GameAudioRequest threat_request = {0};
    failed += assert_true(game_audio_request_pop(&queue, &threat_request) == GAME_AUDIO_REQUEST_RESULT_OK, "threat request popped");
    failed += assert_true(threat_request.sound_id == 4u, "threat mapped to sound id");
    failed += assert_true(threat_request.requested_tick == 14u, "threat source tick tracked");

    failed += assert_true(
        game_audio_request_project_from_event(&(GameEvent){.type = GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED, .payload_size = 0u}, 13u, 9u, &queue)
            == GAME_AUDIO_REQUEST_RESULT_UNKNOWN_EVENT,
        "unknown event rejected"
    );

    GameAudioRequest overflow_request = {0};
    failed += assert_true(game_audio_request_init(&queue, requests, 2u) == GAME_AUDIO_REQUEST_RESULT_OK, "re-init audio queue");
    failed += assert_true(game_audio_request_project_from_event(&event, 14u, 10u, &queue) == GAME_AUDIO_REQUEST_RESULT_OK, "overflow fill first");
    failed += assert_true(game_audio_request_project_from_event(&event, 15u, 11u, &queue) == GAME_AUDIO_REQUEST_RESULT_OK, "overflow fill second");
    failed += assert_true(
        game_audio_request_project_from_event(&event, 16u, 12u, &queue) == GAME_AUDIO_REQUEST_RESULT_QUEUE_FULL,
        "queue overflow rejected"
    );
    failed += assert_true(
        game_audio_request_pop(&queue, &overflow_request) == GAME_AUDIO_REQUEST_RESULT_OK,
        "overflow check preserves deterministic order"
    );
    failed += assert_true(overflow_request.source_event_sequence == 10u, "overflow queue keeps earliest successful request");

    game_audio_request_destroy(&queue);

    if (failed == 0) {
        printf("[audio_request] PASS\n");
    }
    return failed;
}
