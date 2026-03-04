#ifndef __JH_SD_H
#define __JH_SD_H

#include "lib_def.h"


void v_SD_Deinit();

void v_SD_Init();
void v_SD_Test();

bool b_IsMountSD();
bool b_MountSD();
bool b_UnMountSD();

// SD Sensor Log
bool b_SD_Log_Open();
void v_SD_Log_Write(uint8_t* pu8_data, uint16_t u16_len);
void v_SD_Log_Flush();
void v_SD_Log_Close();

#endif


