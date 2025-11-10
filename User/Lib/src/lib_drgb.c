#include "lib_drgb.h"



/*
 * brief	: digital led add to input array
 * date
 * create	: 25.04.15
 * modify	: -
 * note
 * - order 	: Green - Red - Blue
 *
 */
uint16_t _u16_DRGB_Add(uint8_t* pu8_dstArr, uint8_t u8_R, uint8_t u8_G, uint8_t u8_B){
	uint16_t cnt=0;
	pu8_dstArr[cnt++] = u8_G;
	pu8_dstArr[cnt++] = u8_R;
	pu8_dstArr[cnt++] = u8_B;
	return cnt;
}


/*
 * brief	: digital led convert to output array
 * date
 * create	: 25.04.15
 * note
 * - high	: 72	tick
 * - low	: 144	tick
 * - cnt	: srcCnt * 8
 */
uint16_t _u16_DRGB_Out(uint8_t* pu8_dstArr, uint8_t* pu8_srcArr, uint16_t u16_srcCnt, uint8_t u8_high, uint8_t u8_low){
	uint16_t cnt=0;
	for(uint16_t i=0; i<u16_srcCnt; i++){
		for(uint16_t j=0; j<8; ++j){
			if(pu8_srcArr[i] & (0x80 >> j)){
				pu8_dstArr[cnt] = u8_high;
			}
			else{
				pu8_dstArr[cnt] = u8_low;
			}
			++cnt;
		}
	}
	return cnt;
}






