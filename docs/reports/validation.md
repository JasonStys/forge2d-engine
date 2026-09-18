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

The [final GitHub Actions run](https://github.com/JasonStys/forge2d-engine/actions/runs/35323345786)
passed all nine jobs on 2026-09-18:

- GCC and Clang Release builds, tests, demo, benchmark, installation, and CPack archive;
- MSVC Release build, tests, demo, and benchmark;
- at least 80% line and 70% branch coverage over authored engine/platform code;
- Clang AddressSanitizer, UndefinedBehaviorSanitizer, and 20-second parser fuzz smoke;
- pinned SDL3 Linux adapter compilation;
- headless multi-stage container build/smoke;
- clang-tidy plus documentation, source-header, shell-syntax, and generated-index checks.

The coverage artifact reports 781 of 880 lines (88.75%) and 615 of 852 meaningful branches
(72.18%). The separate [CodeQL run](https://github.com/JasonStys/forge2d-engine/actions/runs/35323345714)
also passed for the same commit. Compiler-generated throw and unreachable branches are excluded as
documented in the testing strategy; the XML and detailed HTML report remain attached to the CI run.

## Explicit limitations

The sample does not validate a physics solver, texture pipeline, audio, networking, editor, consoles,
mobile devices, or production/mod security. Local timing is one-host evidence. Exact cross-compiler
hash agreement is not yet asserted.
