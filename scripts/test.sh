#!/usr/bin/env bash
set -euo pipefail

build_config_arg=""
if [ -f build-debug/CMakeCache.txt ] && \
   (grep -q "Visual Studio" build-debug/CMakeCache.txt || grep -q "Xcode" build-debug/CMakeCache.txt); then
  build_config_arg="Debug"
fi

if command -v ctest >/dev/null 2>&1; then
  if [ -n "${build_config_arg}" ]; then
    ctest --test-dir build-debug --build-config "${build_config_arg}" --output-on-failure
  else
    ctest --test-dir build-debug --output-on-failure
  fi
else
  if [ -n "${build_config_arg}" ]; then
    cmake --build build-debug --config "${build_config_arg}" --target test
  else
    cmake --build build-debug --target test
  fi
fi
