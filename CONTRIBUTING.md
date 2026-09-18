# Contributing

## Development contract

Use CMake 3.25+ and a conforming C++20 compiler. Keep the dependency-free core usable without SDL
or a display. New platform integrations belong behind narrow adapters. Do not add copied game art,
private assets, secrets, telemetry, or code from an employer.

Public behavior requires an explanatory comment and tests. Every C, C++, JavaScript, and shell file
needs a header naming the file and purpose; source headers link to the generated code index for
exact symbol locations.

## Required checks

```bash
clang-format --dry-run --Werror <all C/C++ files>
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev --parallel
ctest --test-dir build/dev --output-on-failure
./build/dev/forge2d_benchmark
node scripts/generate-code-index.mjs
node scripts/validate-repository.mjs
node scripts/generate-code-index.mjs --check
```

Run `bash scripts/verify.sh` for the standard full gate. Before merging parser/storage changes, also
run ASan/UBSan and the fuzz target on Clang. CI supplies exact commands.

## Design expectations

- Preserve deterministic iteration or explicitly sort externally visible results.
- State ownership and lifetime; avoid owning raw pointers.
- Validate sizes before allocations and arithmetic.
- Add a reference/property test when optimizing an algorithm.
- Record complexity, benchmark impact, compatibility, and rollback in the pull request.
- Update relevant documentation and generated code index with the code.

## Pull requests

Describe the problem, constraints, alternatives, measurable result, test evidence, compatibility
impact, and rollback. A performance change without a correctness oracle and baseline is incomplete.
