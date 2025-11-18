#ifndef __LIB_LOG_H
#define __LIB_LOG_H

#include <stdio.h>

/**
 * @brief Logging macros for structured debug output
 *
 * These macros provide consistent logging format across the codebase.
 * Format: [LEVEL][MODULE] message
 */

#define LOG_ERROR(module, ...) printf("[ERROR][%s] ", module); printf(__VA_ARGS__); printf("\n")
#define LOG_WARN(module, ...)  printf("[WARN][%s] ", module); printf(__VA_ARGS__); printf("\n")
#define LOG_INFO(module, ...)  printf("[INFO][%s] ", module); printf(__VA_ARGS__); printf("\n")
#define LOG_DEBUG(module, ...) printf("[DEBUG][%s] ", module); printf(__VA_ARGS__); printf("\n")

#endif // __LIB_LOG_H
