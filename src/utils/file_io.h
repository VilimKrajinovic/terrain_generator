#ifndef FILE_IO_H
#define FILE_IO_H

#include "utils/types.h"
#include "memory/arena.h"
#include <stddef.h>

// Read entire file into memory (arena allocated)
// Memory is owned by the arena
// Returns NULL on failure
u8 *file_read_binary_arena(const char *path, size_t *out_size, Arena *arena);

// Read text file into memory (null-terminated, arena allocated)
// Memory is owned by the arena
// Returns NULL on failure
char *file_read_text_arena(const char *path, size_t *out_size, Arena *arena);

// Check if file exists
bool file_exists(const char *path);

// Get file size
size_t file_get_size(const char *path);

#endif // FILE_IO_H
