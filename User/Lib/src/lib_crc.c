#include "lib_crc.h"

uint16_t u16_CRC16_CCITT(const uint8_t* pu8_data, uint32_t u32_len){
	uint16_t u16_crc = 0xFFFFU;

	if(pu8_data == 0) return u16_crc;

	for(uint32_t i = 0; i < u32_len; i++){
		u16_crc ^= (uint16_t)((uint16_t)pu8_data[i] << 8);
		for(int b = 0; b < 8; b++){
			if(u16_crc & 0x8000U)	{u16_crc = (uint16_t)((u16_crc << 1) ^ 0x1021U);}
			else					{u16_crc = (uint16_t)(u16_crc << 1);}
		}
	}
	return u16_crc;
}

int i_CRC16_SelfTest(void){
	static const uint8_t u8_vec[9] = {'1','2','3','4','5','6','7','8','9'};
	return (u16_CRC16_CCITT(u8_vec, sizeof(u8_vec)) == 0x29B1U) ? 1 : 0;
}
