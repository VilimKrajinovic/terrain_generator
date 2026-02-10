#include "arena.h"
#include "core/log.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

// Global scratch arena (single-thread only; do not use concurrently).
// TODO (multi-thread): use thread-local scratch arenas or add locking to avoid
// races/corruption.
static Arena  g_scratch_storage = {};
static Arena *g_scratch_arena   = NULL;

static bool arena_add_overflow(size_t a, size_t b, size_t *out) {
  if(a > SIZE_MAX - b) {
    return true;
  }
  *out = a + b;
  return false;
}

static bool arena_is_power_of_two(size_t value) { return value != 0 && (value & (value - 1)) == 0; }

Arena arena_create(size_t size) {
  Arena arena = {};
  arena.base  = (u8 *)malloc(size);
  if(arena.base) {
    arena.size = size;
    arena.pos  = 0;
    LOG_DEBUG("Arena created: %zu bytes", size);
  } else {
    LOG_ERROR("Failed to create arena: %zu bytes", size);
  }
  return arena;
}

void arena_destroy(Arena *arena) {
  if(arena->base) {
    free(arena->base);
    LOG_DEBUG("Arena destroyed: %zu bytes", arena->size);
  }
  memset(arena, 0, sizeof(Arena));
}

void arena_clear(Arena *arena) {
#ifdef ARENA_DEBUG_FILL
  if(arena->base && arena->size > 0) {
    memset(arena->base, ARENA_DEBUG_FILL_BYTE, arena->size);
  }
#endif
  arena->pos = 0;
}

void *arena_alloc(Arena *arena, size_t size) { return arena_alloc_aligned(arena, size, sizeof(void *)); }

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment) {
  if(alignment == 0 || !arena_is_power_of_two(alignment)) {
    LOG_ERROR("Arena alignment must be a non-zero power of two (got %zu)", alignment);
    return NULL;
  }

  size_t pos_plus = 0;
  if(arena_add_overflow(arena->pos, alignment - 1, &pos_plus)) {
    LOG_ERROR("Arena alignment overflow: pos %zu, alignment %zu", arena->pos, alignment);
    return NULL;
  }

  size_t aligned_pos = pos_plus & ~(alignment - 1);

  // Check if we have enough space
  if(aligned_pos > arena->size || size > arena->size - aligned_pos) {
    LOG_ERROR("Arena out of memory: need %zu bytes, have %zu bytes", size, arena->size - aligned_pos);
    return NULL;
  }

  void *ptr  = arena->base + aligned_pos;
  arena->pos = aligned_pos + size;

  return ptr;
}

ArenaTemp arena_temp_begin(Arena *arena) {
  return ArenaTemp{
    .arena = arena,
    .pos   = arena->pos,
  };
}

void arena_temp_end(ArenaTemp temp) { temp.arena->pos = temp.pos; }

void arena_scratch_init(size_t size) {
  if(g_scratch_arena && g_scratch_arena != &g_scratch_storage) {
    LOG_WARN("Scratch arena already set externally; reinitializing storage scratch");
  }
  g_scratch_storage = arena_create(size);
  g_scratch_arena   = &g_scratch_storage;
  LOG_INFO("Scratch arena initialized: %zu bytes", size);
}

void arena_scratch_shutdown(void) {
  if(!g_scratch_arena) {
    return;
  }
  if(g_scratch_arena == &g_scratch_storage) {
    arena_destroy(g_scratch_arena);
    LOG_INFO("Scratch arena shutdown");
  }
  g_scratch_arena = NULL;
}

ArenaTemp arena_scratch_begin(void) {
  if(!g_scratch_arena || !g_scratch_arena->base) {
    LOG_ERROR("Scratch arena not initialized");
    return ArenaTemp{};
  }
  return arena_temp_begin(g_scratch_arena);
}

void arena_scratch_set(Arena *arena) { g_scratch_arena = arena; }

Arena *arena_scratch_get(void) { return g_scratch_arena; }
