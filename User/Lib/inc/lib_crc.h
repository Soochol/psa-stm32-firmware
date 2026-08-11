#ifndef __JH_LIB_CRC_H
#define __JH_LIB_CRC_H

#include <stdint.h>

// CRC-16/CCITT-FALSE, as required by the SD logging spec section 7:
//   polynomial 0x1021, init 0xFFFF, no input/output reflection, xorout 0x0000.
// Check vector: "123456789" -> 0x29B1.
//
// Bitwise on purpose. A record is 78 bytes at 10 Hz, so this costs roughly
// 0.01% of the core — a lookup table would buy nothing and adds a second place
// for the polynomial to be wrong.
uint16_t u16_CRC16_CCITT(const uint8_t* pu8_data, uint32_t u32_len);

// 1 when the implementation reproduces the check vector. Run once at start-up:
// a wrong CRC here is invisible on the device and only surfaces when the merge
// tool rejects an entire card.
int i_CRC16_SelfTest(void);

#endif
