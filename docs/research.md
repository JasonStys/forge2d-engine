# Research and engineering basis

Sources were reviewed on 2026-09-18. Primary project documentation is used where available.

## SDL3 and renderer boundary

SDL describes itself as a cross-platform low-level library for input, audio, and graphics that is
written in C and works natively with C++. The project therefore uses SDL3 only at the adapter edge;
the portable core remains ordinary C++20. The selected 3.4.16 release is a current stable bugfix
release and its official archive digest is pinned in CMake.

- [SDL3 overview](https://wiki.libsdl.org/SDL3/FrontPage)
- [SDL3 CMake guidance](https://wiki.libsdl.org/SDL3/README-cmake)
- [SDL 3.4.16 release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.16)

SDL's modern GPU API supports explicit device/buffer/texture workflows, but that complexity is not
needed for the primitive sample. The ADR selects SDL Renderer now and preserves a backend seam.

- [SDL3 GPU API](https://wiki.libsdl.org/SDL3/CategoryGPU)

## Dependency acquisition

CMake recommends declaring dependency details before making content available. This project has one
optional external dependency, declared by official release URL and SHA-256, and permits an explicit
local source override for controlled/offline builds.

- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)

## Memory and parser verification

Clang documents AddressSanitizer as compiler/runtime instrumentation for out-of-bounds,
use-after-free, double-free, and related failures, with frame pointers improving traces. CI combines
ASan with UndefinedBehaviorSanitizer for the full suite. LLVM documents libFuzzer as an in-process,
coverage-guided engine that repeatedly calls `LLVMFuzzerTestOneInput`; the scene parser target uses
the recommended combined fuzzer/address/undefined instrumentation and a small valid seed corpus.

- [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [LLVM libFuzzer](https://llvm.org/docs/LibFuzzer.html)

## Static and coverage analysis

Clang-tidy uses the CMake-generated compilation database to diagnose bug-prone, analyzer,
performance, portability, and modernization issues. Gcovr 8.6 produces line/branch XML and HTML
evidence around GCC instrumentation.

- [Clang-tidy documentation](https://clang.llvm.org/extra/clang-tidy/)
- [gcovr 8.6](https://github.com/gcovr/gcovr/releases/tag/8.6)

## Design conclusions

- Keep clocks, devices, and third-party APIs outside canonical simulation.
- Optimize only against a simple correctness oracle where practical.
- Treat asset parsers and allocator arithmetic as security boundaries.
- Pin external sources by immutable digest.
- Separate measured regression budgets from unsupported frame-rate or production claims.
