#ifndef __JH_IO_H
#define __JH_IO_H

#include "main.h"
#include "lib_def.h"



void v_IO_Check_12V();

void v_IO_Enable_12V();
void v_IO_Disable_12V();

void v_IO_Enable_HeatPad();
void v_IO_Disable_HeatPad();  // LOW: Fixed typo (HeadPad -> HeatPad)

void v_IO_Enable_Fan();
void v_IO_Disable_Fan();


void v_IO_Init();

void v_AUDIO_Init();


void v_I2C1_Pin_Deinit();
void v_I2C2_Pin_Deinit();
void v_I2C3_Pin_Deinit();
void v_I2C4_Pin_Deinit();
void v_I2C5_Pin_Deinit();


void v_IO_PWR_WakeUp_Enable();
void v_IO_PWR_WakeUp_Disable();



#endif


