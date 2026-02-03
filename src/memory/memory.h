
#include "memory/arena.h"

typedef struct GameMemory {
  Arena *permanent_memory;
  Arena *transient_memory;
} GameMemory;
