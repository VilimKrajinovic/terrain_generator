#include "log.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

static LogLevel g_min_level = LOG_LEVEL_DEBUG;

static const char *level_strings[] = {
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
};

static const char *level_colors[] = {
  "\033[36m", // Cyan for DEBUG
  "\033[32m", // Green for INFO
  "\033[33m", // Yellow for WARN
  "\033[31m", // Red for ERROR
};

static const char *color_reset = "\033[0m";

void log_init(LogLevel min_level) {
  g_min_level = min_level;
  LOG_INFO("Logging system initialized");
}

void log_shutdown(void) { LOG_INFO("Logging system shutdown"); }

void log_set_level(LogLevel level) { g_min_level = level; }

LogLevel log_get_level(void) { return g_min_level; }

void log_output(LogLevel level, const char *file, int line, const char *fmt, ...) {
  if(level < g_min_level) {
    return;
  }

  // Get timestamp
  time_t     now     = time(NULL);
  struct tm *tm_info = localtime(&now);
  char       time_buf[16];
  strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

  // Extract just the filename from the path
  const char *filename = strrchr(file, '/');
  filename             = filename ? filename + 1 : file;

  // Print log message
  FILE *out = (level >= LOG_LEVEL_WARN) ? stderr : stdout;

  fprintf(
    out, "%s[%s][%s]%s %s:%d: ", level_colors[level], time_buf, level_strings[level], color_reset, filename, line);

  va_list args;
  va_start(args, fmt);
  vfprintf(out, fmt, args);
  va_end(args);

  fprintf(out, "\n");
  fflush(out);
}
