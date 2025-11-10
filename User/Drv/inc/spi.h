#ifndef __JH_SPI_H
#define __JH_SPI_H

#include "main.h"
#include "lib_def.h"



#define ADDR_AUDIO	(0x10 << 1)




uint16_t u16_SPI4_BufGet_EmptyCnt();
int i_SPI4_Write(uint8_t u8_addr, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len, bool b_cont);




#endif


