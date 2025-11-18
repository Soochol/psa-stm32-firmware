#ifndef __LIB_LOG_H
#define __LIB_LOG_H

#include <stdio.h>

// Forward declaration for timestamp function
extern uint32_t u32_Tim_1msGet(void);

/**
 * @brief Logging macros for structured debug output
 *
 * These macros provide consistent logging format across the codebase.
 * Format: [timestamp_ms][LEVEL][MODULE] message
 */

#define LOG_ERROR(module, ...) ((void)0)  // ERROR logs disabled
#define LOG_WARN(module, ...)  ((void)0)  // WARN logs disabled
#define LOG_INFO(module, ...)  ((void)0)  // INFO logs disabled
#define LOG_DEBUG(module, ...) ((void)0)  // DEBUG logs disabled

#endif // __LIB_LOG_H
