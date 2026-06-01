# TICKET ID: AUDIO-001-event-to-audio-request-bus

## Objective

Create a non-authoritative audio request bus derived from simulation events.

## Context

Audio should respond to facts such as noise, impacts, and threat changes without mutating simulation state or becoming required for headless tests.

## Sprint

Sprint 7 - sensory ecology and horde escalation.

## Dependencies

- EVT-003-serializable-event-trace-log
- NOISE-002-noise-event-and-field-impulse

## Acceptance criteria

- Audio request payload includes sound id, origin, priority, source event id, and requested tick.
- Projection from selected event types preserves deterministic order.
- Request queue is fixed-capacity with explicit overflow behavior.
- Headless tests cover projection, ordering, overflow, and source event linking.
- SDL/audio backend remains optional.

## Allowed files

- `src/audio/audio_request.h`
- `src/audio/audio_request.c`
- `tests/test_audio_request.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/02_ARCHITECTURE_MAP.md`
- `docs/12_TESTING.md`

## Out of scope

- Actual sound playback.
- Mixer implementation.
- Asset loading.
- Spatial audio attenuation.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the audio request test target or full `sim_tests` if tests remain single-binary.

## Notes

Audio requests are presentation facts. They must never be consumed by simulation systems as authoritative input.
