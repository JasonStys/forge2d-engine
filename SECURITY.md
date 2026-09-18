# Security policy

## Supported version

The latest `main` revision is supported. This educational engine has no production service-level
commitment.

## Reporting

Use GitHub private vulnerability reporting when available, or contact the repository owner through
the profile contact channel. Do not publish a weaponized scene/replay payload before coordination.
Include the revision, compiler/platform, sanitizer output, reproduction, and impact.

## Trust boundaries

- Scene and replay text are untrusted input: headers, counts, dimensions, identifiers, ranges,
  line lengths, trailing data, and total allocations are bounded before use.
- Entity generations prevent stale handles from silently addressing a recycled entity.
- The C arena checks nulls, power-of-two alignment, integer overflow, and capacity; failure does not
  advance state and no fallback allocation is hidden.
- Spatial-hash output is normalized, deduplicated, sorted, and tested against a brute-force oracle.
- The SDL adapter owns all SDL pointers and is excluded from the dependency-free simulation core.
- CI runs AddressSanitizer, UndefinedBehaviorSanitizer, parser fuzzing, CodeQL, warnings-as-errors,
  dependency review, and cross-compiler builds.

The sanitizers and fuzzer are test tools, not runtime hardening. A real game must additionally
threat-model mod/plugin loading, archives, shaders, save files, network traffic, and update signing.
