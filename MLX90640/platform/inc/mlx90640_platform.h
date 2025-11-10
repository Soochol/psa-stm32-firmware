#ifndef __JH_MLX90640_PLATFORM_H
#define __JH_MLX90640_PLATFORM_H

#include "lib_def.h"

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

void v_IR_TEMP_Test();

void v_TempIR_Deinit();
e_COMM_STAT_t e_IR_Temp_Ready();
void v_IR_TEMP_Handler();

float f_IR_Temp_Get();

void v_Temp_IR_WR_Done();
void v_Temp_IR_RD_Done(uint8_t* pu8, uint16_t u16_cnt);

void v_Temp_IR_Test();

void v_Temp_IR_Tout_Handler();
void v_Temp_IR_Data_Handler();

#endif
