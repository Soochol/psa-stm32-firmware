/*
 * sam_m10q_transport.h
 *
 * SAM-M10Q GPS Module - Transport Layer Abstraction
 * Function pointers for I2C communication
 *
 * Created: 2025-01-11
 */

#ifndef __JH_SAM_M10Q_TRANSPORT_H
#define __JH_SAM_M10Q_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sam_m10q_def.h"

/*
 * Transport Function Pointer Types
 */

// I2C Write function
// return: 0 if successful, -1 if error
typedef int (*_fn_i_SAM_M10Q_Write)(uint8_t u8_addr, uint16_t u16_reg,
                                     uint8_t* pu8_arr, uint16_t u16_len);

// I2C Read function
// return: 0 if successful, -1 if error
typedef int (*_fn_i_SAM_M10Q_Read)(uint8_t u8_addr, uint16_t u16_reg,
                                    uint16_t u16_len);

// Bus status check
// return: 0 if ready, -1 if busy
typedef int (*_fn_i_SAM_M10Q_Bus)(void);

// Get system time (milliseconds)
// return: Current timestamp in ms
typedef uint32_t (*_fn_u32_SAM_M10Q_GetTime)(void);

/*
 * Transport Structure
 */
typedef struct {
    _fn_i_SAM_M10Q_Write    i_write;
    _fn_i_SAM_M10Q_Read     i_read;
    _fn_i_SAM_M10Q_Bus      i_bus;
    _fn_u32_SAM_M10Q_GetTime u32_getTime;
} _x_SAM_M10Q_TRANS_t;

#ifdef __cplusplus
}
#endif

#endif // __JH_SAM_M10Q_TRANSPORT_H
