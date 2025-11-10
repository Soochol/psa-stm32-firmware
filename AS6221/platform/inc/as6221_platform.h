#ifndef __JH_AS6221_PLATFORM_H
#define __JH_AS6221_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "as6221_driver.h"





void v_AS6221_Init_Drvier();

void v_AS6221_Test();


void v_AS6221_Write_DoneHandler(uint8_t u8_addr);
void v_AS6221_Read_DoneHandler(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_len);

void v_Temp_InOut_Deinit();
void v_Temp_InOut_Tout_Handler();
e_COMM_STAT_t e_Temp_InOut_Ready();

void v_Temp_InOut_Handler();

float f_Temp_In_Get();
float f_Temp_Out_Get();



void v_TempOut_Test();

#ifdef __cplusplus
}
#endif

#endif

