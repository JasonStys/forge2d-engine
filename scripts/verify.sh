#!/usr/bin/env bash
# File: verify.sh
# Purpose: Run formatting, static analysis, build, tests, demo, and performance gates locally.
# Commands and variables: see docs/code-index.md; BUILD_DIR contains disposable generated output.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/verify}"

cd "${ROOT_DIR}"
mapfile -t SOURCES < <(find apps benchmarks fuzz include platform src tests \
  -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) | sort)
clang-format --dry-run --Werror "${SOURCES[@]}"
cmake -S . -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel
clang-tidy -p "${BUILD_DIR}" src/collision.cpp src/pathfinding.cpp src/replay.cpp src/scene.cpp \
  src/software_renderer.cpp src/world.cpp apps/forge2d_arena.cpp tests/test_main.cpp \
  benchmarks/benchmark.cpp
ctest --test-dir "${BUILD_DIR}" --output-on-failure
"${BUILD_DIR}/forge2d_benchmark" | tee docs/reports/generated/benchmark.json
"${BUILD_DIR}/forge2d_arena" assets/arena.scene out/verify
node scripts/validate-repository.mjs
node scripts/generate-code-index.mjs --check
cmake --install "${BUILD_DIR}" --prefix "${BUILD_DIR}/install"
