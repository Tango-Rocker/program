#!/usr/bin/env bash
set -euo pipefail

if command -v clang-format >/dev/null 2>&1; then
  find src tests include docs -type f \( -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
else
  echo "clang-format not found; skipping."
fi
