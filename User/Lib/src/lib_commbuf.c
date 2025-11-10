#include "lib_commbuf.h"





bool _b_CommBuf_Put(_x_COMM_BUF_t* px, uint8_t u8_addr, \
					uint8_t u8_reg, uint8_t* pu8_data, uint16_t u16_cnt, bool b_read){
	px->u8_addr = u8_addr;
	px->u8_reg = u8_reg;
	px->pu8_data = pu8_data;
	px->u16_cnt = u16_cnt;
	px->b_read = b_read;

	return false;
}




