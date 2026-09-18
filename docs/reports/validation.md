# Validation report

## Completed locally

- Dependency-free C/C++ core configures and builds with MSVC 19.51 under warnings-as-errors.
- Eight deterministic test groups pass, including randomized/reference and negative cases.
- Headless scene-to-replay-to-render integration produces documented state and pixel hashes.
- Performance executable passes all three budgets and emits machine-readable JSON.
- The official SDL 3.4.16 archive SHA-256 matches the pinned digest, and the optional Windows SDL
  target compiles and links successfully.
- Arena/parser bounds, entity generations, stable collision/path ordering, and explicit ownership
  are implemented and documented.
- Clang-tidy 22.1.3 passes the authored headless targets with warnings promoted to errors.

## CI validation contract

GitHub Actions must independently pass:

- GCC and Clang Release builds, tests, demo, benchmark, installation, and CPack archive;
- MSVC Release build, tests, demo, and benchmark;
- at least 80% line and 70% branch coverage over authored engine/platform code;
- Clang AddressSanitizer, UndefinedBehaviorSanitizer, and 20-second parser fuzz smoke;
- pinned SDL3 Linux adapter compilation;
- headless multi-stage container build/smoke;
- clang-tidy, documentation/header/code-index checks, dependency review, and C/C++ CodeQL.

## Explicit limitations

The sample does not validate a physics solver, texture pipeline, audio, networking, editor, consoles,
mobile devices, or production/mod security. Local timing is one-host evidence. Exact cross-compiler
hash agreement is not yet asserted. The first cloud workflow establishes Linux/coverage/sanitizer
evidence and is linked from the final handoff.
