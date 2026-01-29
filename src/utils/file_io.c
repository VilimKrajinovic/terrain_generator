#include "file_io.h"
#include "core/log.h"
#include <stdio.h>
#include <sys/stat.h>

u8 *file_read_binary_arena(const char *path, size_t *out_size, Arena *arena) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        LOG_ERROR("Failed to open file: %s", path);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer from arena
    u8 *buffer = ARENA_PUSH_ARRAY(arena, u8, size);
    if (!buffer) {
        LOG_ERROR("Failed to allocate memory for file: %s", path);
        fclose(file);
        return NULL;
    }

    // Read file
    size_t read = fread(buffer, 1, size, file);
    if (read != size) {
        LOG_ERROR("Failed to read file: %s (read %zu of %zu bytes)", path, read, size);
        fclose(file);
        return NULL;
    }

    fclose(file);

    if (out_size) {
        *out_size = size;
    }

    LOG_DEBUG("Read %zu bytes from: %s", size, path);
    return buffer;
}

char *file_read_text_arena(const char *path, size_t *out_size, Arena *arena) {
    FILE *file = fopen(path, "r");
    if (!file) {
        LOG_ERROR("Failed to open file: %s", path);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer from arena (+ 1 for null terminator)
    char *buffer = ARENA_PUSH_ARRAY(arena, char, size + 1);
    if (!buffer) {
        LOG_ERROR("Failed to allocate memory for file: %s", path);
        fclose(file);
        return NULL;
    }

    // Read file
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';

    fclose(file);

    if (out_size) {
        *out_size = read;
    }

    LOG_DEBUG("Read %zu bytes from: %s", read, path);
    return buffer;
}

bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

size_t file_get_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return (size_t)st.st_size;
}
