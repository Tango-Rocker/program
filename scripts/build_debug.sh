#!/usr/bin/env bash
set -euo pipefail

jobs="${1:-}"
if [ -z "${jobs}" ]; then
  if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
  elif command -v getconf >/dev/null 2>&1; then
    jobs="$(getconf _NPROCESSORS_ONLN)"
  else
    jobs=2
  fi
fi

cmake --build build-debug -- -j"$jobs"
