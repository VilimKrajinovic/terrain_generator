#include "arena.h"
#include "core/log.h"
#include <stdlib.h>
#include <string.h>

// Global scratch arena
static Arena g_scratch_arena = {0};

Arena arena_create(size_t size) {
    Arena arena = {0};
    arena.base = (u8 *)malloc(size);
    if (arena.base) {
        arena.size = size;
        arena.pos = 0;
        LOG_DEBUG("Arena created: %zu bytes", size);
    } else {
        LOG_ERROR("Failed to create arena: %zu bytes", size);
    }
    return arena;
}

void arena_destroy(Arena *arena) {
    if (arena->base) {
        free(arena->base);
        LOG_DEBUG("Arena destroyed: %zu bytes", arena->size);
    }
    memset(arena, 0, sizeof(Arena));
}

void arena_clear(Arena *arena) {
    arena->pos = 0;
}

void *arena_alloc(Arena *arena, size_t size) {
    return arena_alloc_aligned(arena, size, sizeof(void *));
}

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment) {
    // Align current position
    size_t aligned_pos = (arena->pos + alignment - 1) & ~(alignment - 1);

    // Check if we have enough space
    if (aligned_pos + size > arena->size) {
        LOG_ERROR("Arena out of memory: need %zu bytes, have %zu bytes",
                  size, arena->size - arena->pos);
        return NULL;
    }

    void *ptr = arena->base + aligned_pos;
    arena->pos = aligned_pos + size;

    return ptr;
}

ArenaTemp arena_temp_begin(Arena *arena) {
    return (ArenaTemp){
        .arena = arena,
        .pos = arena->pos,
    };
}

void arena_temp_end(ArenaTemp temp) {
    temp.arena->pos = temp.pos;
}

void arena_scratch_init(size_t size) {
    g_scratch_arena = arena_create(size);
    LOG_INFO("Scratch arena initialized: %zu bytes", size);
}

void arena_scratch_shutdown(void) {
    arena_destroy(&g_scratch_arena);
    LOG_INFO("Scratch arena shutdown");
}

ArenaTemp arena_scratch_begin(void) {
    return arena_temp_begin(&g_scratch_arena);
}
