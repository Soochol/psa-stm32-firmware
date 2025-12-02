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
 *
 * Set LOG_ENABLE to 1 to enable all logs, 0 to disable
 */

#define LOG_ENABLE 0

#if LOG_ENABLE
    #define LOG_ERROR(module, ...) do { printf("[%lu][ERROR][%s] ", u32_Tim_1msGet(), module); printf(__VA_ARGS__); printf("\r\n"); } while(0)
    #define LOG_WARN(module, ...)  do { printf("[%lu][WARN][%s] ", u32_Tim_1msGet(), module); printf(__VA_ARGS__); printf("\r\n"); } while(0)
    #define LOG_INFO(module, ...)  do { printf("[%lu][INFO][%s] ", u32_Tim_1msGet(), module); printf(__VA_ARGS__); printf("\r\n"); } while(0)
    #define LOG_DEBUG(module, ...) do { printf("[%lu][DEBUG][%s] ", u32_Tim_1msGet(), module); printf(__VA_ARGS__); printf("\r\n"); } while(0)
#else
    #define LOG_ERROR(module, ...) ((void)0)
    #define LOG_WARN(module, ...)  ((void)0)
    #define LOG_INFO(module, ...)  ((void)0)
    #define LOG_DEBUG(module, ...) ((void)0)
#endif

#endif // __LIB_LOG_H
