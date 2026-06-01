#include <stdio.h>
#include <string.h>

#include "event/event.h"
#include "event/event_queue.h"
#include "render/particle_request.h"
#include "sim/projectile.h"
#include "sim/status_effect.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[particle_request] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_particle_request(void) {
    int failed = 0;

    GameParticleRequestQueue queue = {0};
    GameParticleRequest entries[2] = {0};
    failed += assert_true(game_particle_request_init(&queue, entries, 2u) == GAME_PARTICLE_REQUEST_RESULT_OK, "particle queue init");

    GameProjectileImpactPayload impact_payload = {
        .projectile_id = 5u,
        .source_index = 1u,
        .source_generation = 1u,
        .target_q = 3,
        .target_r = 4,
        .damage = 7,
        .ability_id = 2u,
    };
    GameEvent impact_event = {
        .type = GAME_EVENT_TYPE_PROJECTILE_IMPACT,
        .tick = 1u,
        .source = {1u, 1u},
        .payload_size = sizeof(impact_payload),
    };
    memcpy(impact_event.payload.bytes, &impact_payload, sizeof(impact_payload));
    failed += assert_true(
        game_particle_request_project_from_event(&impact_event, 12u, 3u, &queue) == GAME_PARTICLE_REQUEST_RESULT_OK,
        "project from impact"
    );

    GameStatusEffectPayload status_payload = {
        .effect_id = 9u,
        .magnitude = 4,
        .duration_ticks = 3u,
        .target_q = -2,
        .target_r = 1,
    };
    GameEvent status_event = {
        .type = GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED,
        .tick = 2u,
        .source = {2u, 2u},
        .payload_size = sizeof(status_payload),
    };
    memcpy(status_event.payload.bytes, &status_payload, sizeof(status_payload));
    failed += assert_true(
        game_particle_request_project_from_event(&status_event, 13u, 4u, &queue) == GAME_PARTICLE_REQUEST_RESULT_OK,
        "project from status"
    );

    GameParticleRequest first = {0};
    failed += assert_true(game_particle_request_pop(&queue, &first) == GAME_PARTICLE_REQUEST_RESULT_OK, "particle first pop");
    failed += assert_true(first.effect_id == 1u, "impact maps to effect id");
    failed += assert_true(first.intensity == (uint32_t)(impact_payload.damage > 0 ? impact_payload.damage : 1), "impact intensity");

    GameParticleRequest second = {0};
    failed += assert_true(game_particle_request_pop(&queue, &second) == GAME_PARTICLE_REQUEST_RESULT_OK, "particle second pop");
    failed += assert_true(second.effect_id == 2u, "status maps to effect id");
    failed += assert_true(second.duration_ticks == status_payload.duration_ticks + 1u, "status duration passed");

    GameEvent unknown = {.type = GAME_EVENT_TYPE_NOISE_EMITTED, .payload_size = 0u};
    failed += assert_true(
        game_particle_request_project_from_event(&unknown, 0u, 0u, &queue) == GAME_PARTICLE_REQUEST_RESULT_UNKNOWN_EVENT,
        "unknown event rejected"
    );

    GameEvent short_payload = {
        .type = GAME_EVENT_TYPE_PROJECTILE_IMPACT,
        .payload_size = 1u,
    };
    failed += assert_true(
        game_particle_request_project_from_event(&short_payload, 14u, 0u, &queue) == GAME_PARTICLE_REQUEST_RESULT_BAD_PAYLOAD,
        "bad payload rejected"
    );

    game_particle_request_destroy(&queue);

    if (failed == 0) {
        printf("[particle_request] PASS\n");
    }
    return failed;
}
