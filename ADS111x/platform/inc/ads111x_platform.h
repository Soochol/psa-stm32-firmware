#ifndef __JH_ADS111X_PLATFORM_H
#define __JH_ADS111X_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ads111x_drv.h"


void v_ADS111X_WrDone(uint8_t u8_addr);
void v_ADS111X_RdDone(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_cnt);

void v_FSR_Deinit();
e_COMM_STAT_t e_FSR_Ready();
void v_FSR_Data_Handler();
void v_FSR_Tout_Handler();

float f_FSR_Get_Left();
float f_FSR_Get_Right();

uint16_t u16_FSR_Get_Left();
uint16_t u16_FSR_Get_Right();


#ifdef __cplusplus
}
#endif

#endif

