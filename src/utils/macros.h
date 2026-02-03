#ifndef MACROS_H
#define MACROS_H

#include <vulkan/vulkan.h>
#include "core/log.h"

// Array size macro
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Vulkan error checking macro
#define VK_CHECK(call) do { \
    VkResult _result = (call); \
    if (_result != VK_SUCCESS) { \
        LOG_ERROR("Vulkan error %d at %s:%d", _result, __FILE__, __LINE__); \
        return _result; \
    } \
} while (0)

// Vulkan check with custom return value
#define VK_CHECK_RETURN(call, ret) do { \
    VkResult _result = (call); \
    if (_result != VK_SUCCESS) { \
        LOG_ERROR("Vulkan error %d at %s:%d", _result, __FILE__, __LINE__); \
        return (ret); \
    } \
} while (0)

// Null check macro
#define NULL_CHECK(ptr) do { \
    if ((ptr) == NULL) { \
        LOG_ERROR("Null pointer at %s:%d", __FILE__, __LINE__); \
        return RESULT_ERROR_GENERIC; \
    } \
} while (0)

// Min/Max macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) (MAX(min, MIN(max, val)))

// Unused parameter macro
#define UNUSED(x) (void)(x)

#define KILOBYTES(Value) ((Value) * 1024)
#define MEGABYTES(Value) (KILOBYTES(Value) * 1024)
#define GIGABYTES(Value) (MEGABYTES(Value) * 1024)


#if DEBUG
#define ASSERT(Predicate) if(!(Predicate)) {* (int *)0 = 0;}
#else
#define ASSERT(Predicate)
#endif

#endif // MACROS_H
