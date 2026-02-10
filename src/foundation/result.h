#ifndef RESULT_H
#define RESULT_H

// Result codes for internal functions
typedef enum Result {
  RESULT_SUCCESS              = 0,
  RESULT_ERROR_GENERIC        = -1,
  RESULT_ERROR_OUT_OF_MEMORY  = -2,
  RESULT_ERROR_FILE_NOT_FOUND = -3,
  RESULT_ERROR_VULKAN         = -4,
  RESULT_ERROR_WINDOW         = -5,
  RESULT_ERROR_DEVICE         = -6,
} Result;

#endif // RESULT_H
