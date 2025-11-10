#ifndef __JH_SD_H
#define __JH_SD_H

#include "lib_def.h"


void v_SD_Deinit();

void v_SD_Init();
void v_SD_Test();

bool b_IsMountSD();
bool b_MountSD();
bool b_UnMountSD();

#endif


