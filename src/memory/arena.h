#ifndef ARENA_H
#define ARENA_H

#include "utils/types.h"
#include <stddef.h>

#ifdef __cplusplus
#define ALIGNOF_TYPE(type) alignof(type)
#else
#define ALIGNOF_TYPE(type) _Alignof(type)
#endif

// Memory arena for fast bump allocation
typedef struct Arena {
  u8 *base;
  size_t size;
  size_t pos;
} Arena;

// Temporary arena scope for restoring position
typedef struct ArenaTemp {
  Arena *arena;
  size_t pos; // Saved position for scope restoration
} ArenaTemp;

// Core arena API
Arena arena_create(size_t size);
void arena_destroy(Arena *arena);
void arena_clear(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);
void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);

// Temporary/scratch arena API
ArenaTemp arena_temp_begin(Arena *arena);
void arena_temp_end(ArenaTemp temp);

// Optional debug fill when clearing arenas. Define ARENA_DEBUG_FILL to enable.
#ifndef ARENA_DEBUG_FILL_BYTE
#define ARENA_DEBUG_FILL_BYTE 0xDD
#endif

// Global scratch arena for temporary allocations
void arena_scratch_init(size_t size);
void arena_scratch_shutdown(void);
ArenaTemp arena_scratch_begin(void);
void arena_scratch_set(Arena *arena);
Arena *arena_scratch_get(void);

// Helper macros for typed allocation
#define ARENA_PUSH_STRUCT(arena, type)                                        \
  ((type *)arena_alloc_aligned((arena), sizeof(type), ALIGNOF_TYPE(type)))

#define ARENA_PUSH_ARRAY(arena, type, count)                                  \
  ((type *)arena_alloc_aligned(                                               \
    (arena), sizeof(type) * (count), ALIGNOF_TYPE(type)))

#endif // ARENA_H
