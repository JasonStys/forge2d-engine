/*
 * File: arena.c
 * Purpose: Implement a bounds-checked, overflow-aware C17 linear allocator.
 * Symbols and line locations: see docs/code-index.md; allocation is O(1) with no hidden fallback.
 */
#include "arena.h"

#include <string.h>

void forge2d_arena_init(forge2d_arena* arena, void* memory, size_t capacity) {
    if (arena == NULL) {
        return;
    }
    arena->memory = (uint8_t*)memory;
    arena->capacity = memory == NULL ? 0U : capacity;
    arena->offset = 0U;
    arena->peak = 0U;
}

void* forge2d_arena_allocate(forge2d_arena* arena, size_t size, size_t alignment) {
    size_t aligned_offset;
    size_t mask;
    void* result;

    if (arena == NULL || arena->memory == NULL || size == 0U || alignment == 0U ||
        (alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }
    mask = alignment - 1U;
    if (arena->offset > SIZE_MAX - mask) {
        return NULL;
    }
    aligned_offset = (arena->offset + mask) & ~mask;
    if (aligned_offset > arena->capacity || size > arena->capacity - aligned_offset) {
        return NULL;
    }
    result = arena->memory + aligned_offset;
    memset(result, 0, size);
    arena->offset = aligned_offset + size;
    if (arena->offset > arena->peak) {
        arena->peak = arena->offset;
    }
    return result;
}

void forge2d_arena_reset(forge2d_arena* arena) {
    if (arena != NULL) {
        arena->offset = 0U;
    }
}
