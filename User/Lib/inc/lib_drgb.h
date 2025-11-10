#ifndef __JH_LIB_DRGB_H
#define __JH_LIB_DRGB_H

#include "lib_def.h"


uint16_t _u16_DRGB_Add(uint8_t* pu8_dstArr, uint8_t u8_R, uint8_t u8_G, uint8_t u8_B);
uint16_t _u16_DRGB_Out(uint8_t* pu8_dstArr, uint8_t* pu8_srcArr, uint16_t u16_srcCnt, uint8_t u8_high, uint8_t u8_low);


#endif


