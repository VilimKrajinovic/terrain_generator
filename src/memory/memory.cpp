#include "memory.h"
#include "core/log.h"
#include <string.h>

static bool arena_create_checked(Arena *arena, size_t size) {
  if(size == 0) {
    memset(arena, 0, sizeof(*arena));
    return true;
  }
  *arena = arena_create(size);
  return arena->base != NULL;
}

bool memory_init(MemoryContext *memory, const MemoryConfig *config) {
  if(!memory || !config) {
    return false;
  }

  memset(memory, 0, sizeof(*memory));

  if(!arena_create_checked(&memory->arenas[MEMORY_ARENA_PERMANENT], config->permanent_size)) {
    LOG_ERROR("Failed to create permanent arena");
    goto fail;
  }

  if(!arena_create_checked(&memory->arenas[MEMORY_ARENA_TRANSIENT], config->transient_size)) {
    LOG_ERROR("Failed to create transient arena");
    goto fail;
  }

  if(!arena_create_checked(&memory->arenas[MEMORY_ARENA_FRAME], config->frame_size)) {
    LOG_ERROR("Failed to create frame arena");
    goto fail;
  }

  if(!arena_create_checked(&memory->arenas[MEMORY_ARENA_SCRATCH], config->scratch_size)) {
    LOG_ERROR("Failed to create scratch arena");
    goto fail;
  }

  if(memory->arenas[MEMORY_ARENA_SCRATCH].base) {
    arena_scratch_set(&memory->arenas[MEMORY_ARENA_SCRATCH]);
  }

  memory->initialized = true;
  return true;

fail:
  memory_shutdown(memory);
  return false;
}

void memory_shutdown(MemoryContext *memory) {
  if(!memory) {
    return;
  }

  if(arena_scratch_get() == &memory->arenas[MEMORY_ARENA_SCRATCH]) {
    arena_scratch_set(NULL);
  }

  for(u32 i = 0; i < MEMORY_ARENA_COUNT; i++) {
    if(memory->arenas[i].base) {
      arena_destroy(&memory->arenas[i]);
    }
  }

  memset(memory, 0, sizeof(*memory));
}

Arena *memory_arena(MemoryContext *memory, MemoryArenaId id) {
  if(!memory || id >= MEMORY_ARENA_COUNT) {
    return NULL;
  }
  return &memory->arenas[id];
}

void memory_begin_frame(MemoryContext *memory) {
  if(!memory) {
    return;
  }

  Arena *frame_arena = &memory->arenas[MEMORY_ARENA_FRAME];
  if(frame_arena->base) {
    arena_clear(frame_arena);
  }
}

ArenaTemp memory_scratch_begin(MemoryContext *memory) {
  if(!memory) {
    return arena_scratch_begin();
  }

  Arena *scratch_arena = &memory->arenas[MEMORY_ARENA_SCRATCH];
  if(!scratch_arena->base) {
    return arena_scratch_begin();
  }

  return arena_temp_begin(scratch_arena);
}

// If setup, returns the permanent arena pointer
Arena *permanent_memory(MemoryContext *memory) {
  if(!memory) {
    return NULL;
  }
  return &memory->arenas[MEMORY_ARENA_PERMANENT];
}

Arena *transient_memory(MemoryContext *memory) {
  if(!memory) {
    return NULL;
  }
  return &memory->arenas[MEMORY_ARENA_TRANSIENT];
}

Arena *frame_memory(MemoryContext *memory) {
  if(!memory) {
    return NULL;
  }
  return &memory->arenas[MEMORY_ARENA_FRAME];
}

Arena *scratch_memory(MemoryContext *memory) {
  if(!memory) {
    return NULL;
  }
  return &memory->arenas[MEMORY_ARENA_SCRATCH];
}
