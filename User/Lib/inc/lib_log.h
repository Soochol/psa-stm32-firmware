#ifndef __LIB_LOG_H
#define __LIB_LOG_H

#include <stdio.h>
#include "SEGGER_RTT.h"

// Log level: 0=OFF, 1=ERR, 2=WARN, 3=INFO, 4=DBG
// Set via build flag: -DLOG_LEVEL=3
#ifndef LOG_LEVEL
  #define LOG_LEVEL  1
#endif

#if LOG_LEVEL >= 1
  #define LOG_ERROR(mod, fmt, ...) \
    SEGGER_RTT_printf(0, "[E/%s]" fmt "\r\n", mod, ##__VA_ARGS__)
#else
  #define LOG_ERROR(mod, ...) ((void)0)
#endif

#if LOG_LEVEL >= 2
  #define LOG_WARN(mod, fmt, ...) \
    SEGGER_RTT_printf(0, "[W/%s]" fmt "\r\n", mod, ##__VA_ARGS__)
#else
  #define LOG_WARN(mod, ...) ((void)0)
#endif

#if LOG_LEVEL >= 3
  #define LOG_INFO(mod, fmt, ...) \
    SEGGER_RTT_printf(0, "[I/%s]" fmt "\r\n", mod, ##__VA_ARGS__)
#else
  #define LOG_INFO(mod, ...) ((void)0)
#endif

#if LOG_LEVEL >= 4
  #define LOG_DEBUG(mod, fmt, ...) \
    SEGGER_RTT_printf(0, "[D/%s]" fmt "\r\n", mod, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(mod, ...) ((void)0)
#endif

#endif // __LIB_LOG_H
