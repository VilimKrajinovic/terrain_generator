#ifndef MEMORY_H
#define MEMORY_H

#include "memory/arena.h"
#include "utils/types.h"

typedef enum MemoryArenaId {
  MEMORY_ARENA_PERMANENT = 0,
  MEMORY_ARENA_TRANSIENT,
  MEMORY_ARENA_FRAME,
  MEMORY_ARENA_SCRATCH,
  MEMORY_ARENA_COUNT
} MemoryArenaId;

typedef struct MemoryConfig {
  size_t permanent_size;
  size_t transient_size;
  size_t frame_size;
  size_t scratch_size;
} MemoryConfig;

typedef struct MemoryContext {
  Arena arenas[MEMORY_ARENA_COUNT];
  bool initialized;
} MemoryContext;

bool memory_init(MemoryContext *memory, const MemoryConfig *config);
void memory_shutdown(MemoryContext *memory);

Arena *memory_arena(MemoryContext *memory, MemoryArenaId id);
void memory_begin_frame(MemoryContext *memory);
ArenaTemp memory_scratch_begin(MemoryContext *memory);

Arena *permanent_memory(MemoryContext *memory);
Arena *transient_memory(MemoryContext *memory);
Arena *frame_memory(MemoryContext *memory);
Arena *scratch_memory(MemoryContext *memory);

#endif // MEMORY_H
