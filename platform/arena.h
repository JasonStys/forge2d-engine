/*
 * File: arena.h
 * Purpose: Expose a narrow C17 linear-arena boundary usable by the C++ engine and C callers.
 * Symbols and line locations: see docs/code-index.md; forge2d_arena owns capacity, offset, and
 * peak.
 */
#ifndef FORGE2D_ARENA_H
#define FORGE2D_ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct forge2d_arena {
    uint8_t* memory;
    size_t capacity;
    size_t offset;
    size_t peak;
} forge2d_arena;

/** Initialize an arena over caller-owned memory; no allocation or ownership transfer occurs. */
void forge2d_arena_init(forge2d_arena* arena, void* memory, size_t capacity);

/** Return aligned zeroed storage, or NULL without changing offset when the request cannot fit. */
void* forge2d_arena_allocate(forge2d_arena* arena, size_t size, size_t alignment);

/** Reuse all storage while retaining the high-water mark for profiling evidence. */
void forge2d_arena_reset(forge2d_arena* arena);

#ifdef __cplusplus
}
#endif

#endif
