#ifndef __JH_LIB_SSF_H
#define __JH_LIB_SSF_H

#include "lib_def.h"



#define SSF_PRECISION	float





uint16_t _u16_SSF_Handler(SSF_PRECISION* raw, uint16_t u16_rawSize, uint16_t u16_windSize, uint16_t* pu16_peakPos, uint16_t u16_posSize);



#endif


