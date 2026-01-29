#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// Log levels
typedef enum LogLevel {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_ERROR = 3,
  LOG_LEVEL_NONE = 4,
} LogLevel;

// Initialize logging system
void log_init(LogLevel min_level);

// Shutdown logging system
void log_shutdown(void);

// Set minimum log level
void log_set_level(LogLevel level);

// Get current log level
LogLevel log_get_level(void);

// Core logging function
void log_output(LogLevel level, const char *file, int line, const char *fmt,
                ...);

// Convenience macros
// Note: Using GNU extension ##__VA_ARGS__ for compatibility with zero variadic
// arguments
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define LOG_DEBUG(fmt, ...)                                                    \
  log_output(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
  log_output(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                     \
  log_output(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
  log_output(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif // LOG_H
