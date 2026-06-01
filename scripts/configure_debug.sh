#!/usr/bin/env bash
set -euo pipefail

# Keep SDL optional: default to ON, but users can override via args.
game_sdl="${GAME_ENABLE_SDL:-ON}"
game_tests="${GAME_ENABLE_TESTS:-ON}"

cmake -S . -B build-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DGAME_ENABLE_SDL="${game_sdl}" \
  -DGAME_ENABLE_TESTS="${game_tests}" \
  "$@"
