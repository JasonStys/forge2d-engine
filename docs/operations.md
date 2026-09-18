# Build, release, and troubleshooting runbook

## Supported configurations

- Core: CMake 3.25+, C++20/C17, MSVC/GCC/Clang.
- Optional presentation: SDL 3.4.16 fetched from the official archive with SHA-256 verification.
- Headless container: Ubuntu 24.04 build and runtime stages, non-root UID 10001.

## Standard build

```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
ctest --test-dir build/release --output-on-failure
cmake --install build/release --prefix dist/install
(cd build/release && cpack -G TGZ)
```

## SDL dependency recovery

If SDL download fails, verify network/TLS configuration and the official release URL. Do not bypass
hash verification. An approved local source tree can be supplied without changing the project:

```bash
cmake -S . -B build/sdl -G Ninja -DFORGE2D_BUILD_SDL=ON \
  -DFETCHCONTENT_SOURCE_DIR_SDL3=/absolute/path/to/SDL3-3.4.16
```

The source must match SHA-256
`7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68`.

## Regression response

| Symptom | Checks | Action |
|---|---|---|
| Replay hash changed | scene/replay bytes, tick count, snapshot ordering, integer semantics | inspect diff; update evidence only for intentional reviewed changes |
| Collision oracle fails | cell coverage, negative floor division, pair normalization, exact overlap | stop optimization; minimize random seed case |
| Parser fuzz crash | sanitizer trace and saved corpus artifact | add regression seed/test before fix |
| Benchmark fails | Release build, host load, entity distribution, candidate pair count | rerun; profile; revert algorithmic regression |
| SDL build fails | pinned source hash, SDL release notes, adapter API | keep core green; update adapter/version in reviewed change |
| Sanitizer fails | first report and symbolized stack | treat as correctness failure; do not suppress without root cause |

## Release checklist

1. Confirm format/static/compiler/test/coverage/sanitizer/fuzz/CodeQL jobs are green.
2. Compare benchmark JSON and headless state/pixel hashes with the prior release.
3. Build the SDL target on at least Windows and Linux.
4. Inspect installed headers/libraries and CPack archive contents.
5. Update version, changelog/release notes, compatibility, and rollback instructions.
6. Tag only the reviewed commit; retain workflow artifacts with the release record.

## Rollback

Reinstall the previous archive or reset the consuming project's version pin. Scene/replay format one
remains fail-closed: do not make a newer incompatible file masquerade as version one. Restore both
engine binary and matching assets/replay fixtures when investigating deterministic regressions.
