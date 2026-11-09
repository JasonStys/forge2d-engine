# Collision category/mask workflow validation — 2026-11-09

Jason Stys: deterministic game-engine collision selection and migration evidence.

Bodies and raw collision items now carry one category bit and an inclusion mask.
Both actors must include the other's category. Zero masks intentionally disable
matching. The optimized spatial query and independent quadratic reference apply
their own inclusion predicates; World propagates body metadata unchanged. Drawing,
position integration and boundary response stay independent of filtering.

Scene writers use v2; v1 remains readable with category1/all mask bits. Both filter
words belong to state hashing, intentionally changing old numeric fingerprints.
The replay input format is unchanged. Unknown versions and bad/missing/negative/
overflowing filter fields remain errors. A v2 fuzz seed exercises the new grammar.

## Observed local evidence

- MSVC warning-as-error release build and10/10 native test groups passed.
  150 fixed-seed50-item scenes with varied masks/categories exactly match the
  independent quadratic oracle. A hand matrix requires exactly player/character
  and player/zone matches; reverse ordering, bit31, zero masks, invalid categories,
  real World propagation, invalid-spawn atomicity and v1/v2 round trips are tested.
- Both actual CLI demonstrations completed360ticks. The five-entity filtered
  example yielded exactly two initial pairs and zero final pairs; CSV rows list
  the correct category/mask values. Canonical scene, replay and PPM were produced.
  Initial/final are two snapshots, not collision episode/trajectory evidence.
- Existing20,000-entity benchmark passed unchanged budgets: storage1.267ms,
  broadphase7.181ms, simulation4.651ms on this PC. Timings are not real-time claims.
- Formatting and generated source-index/repository checks passed. A native
  header/library/CMake-target install into a fresh project-local prefix passed.
  Optional SDL3.4.16 frontend rebuilt from the existing pinned local source.
  Warning-denying Clang-tidy passed on all five changed implementation/test units;
  existing vendor-header warnings remained outside the configured user-code scope.
  Its first attempt lacked standard-library include paths; the identical policy
  passed under the installed Visual Studio developer environment. Generated PPM
  pixels were viewed through a lossless PNG companion: the player moves while
  stationary overlapping actors remain drawn, independently of query filtering.
  Linux sanitizer/fuzz/coverage and compiler matrix remain mandatory publication
  gates; no acceptance limit or independent reference was removed.

## Limits

This is a fictional integer2D game-engine query feature, with no real-world
tracking, targeting, flight, physics calibration or weapon control. Filtering
does not prevent bodies from passing through each other, apply forces or supply
enter/exit trigger callbacks. Consumers own gameplay reactions and must pass
currently valid handles to raw queries. State hashes are deterministic fingerprints,
not cryptographic attestation. No user scenes, existing outputs or services were
deleted. Parent owns remote CI/publication; no Actions triggered here.
