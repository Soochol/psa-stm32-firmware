#ifndef __JH_LIB_COMMBUF_H
#define __JH_LIB_COMMBUF_H

#include "lib_def.h"


typedef struct {
	uint8_t u8_addr;
	uint8_t u8_reg;
	uint8_t* pu8_data;
	uint16_t u16_cnt;
	bool b_read;
	uint16_t u16_num;
} _x_COMM_BUF_t;


bool _b_CommBuf_Put(_x_COMM_BUF_t* px, uint8_t u8_addr, \
					uint8_t u8_reg, uint8_t* pu8_data, uint16_t u16_cnt, bool b_read);





#endif


