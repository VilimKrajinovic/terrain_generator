#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Common type aliases
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

// Vertex structure for rendering
typedef struct Vertex {
    f32 position[3];
    f32 color[3];
} Vertex;

// Result codes for internal functions
typedef enum Result {
    RESULT_SUCCESS = 0,
    RESULT_ERROR_GENERIC = -1,
    RESULT_ERROR_OUT_OF_MEMORY = -2,
    RESULT_ERROR_FILE_NOT_FOUND = -3,
    RESULT_ERROR_VULKAN = -4,
    RESULT_ERROR_WINDOW = -5,
    RESULT_ERROR_DEVICE = -6,
} Result;

#endif // TYPES_H
