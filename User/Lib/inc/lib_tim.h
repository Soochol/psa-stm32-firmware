#ifndef __JH_LIB_TIM_H
#define __JH_LIB_TIM_H

#include "lib_def.h"


bool _b_Tim_Is_OVR(uint32_t u32_tick, uint32_t u32_ref, uint32_t u32_itv);
bool _b_Tim_Is_OVR_withERR(uint32_t u32_tick, uint32_t u32_ref, uint32_t u32_itv, uint32_t u32_err);
uint32_t _u32_Tim_Is_ITV(uint32_t u32_tick, uint32_t u32_ref);
void _v_Tim_Delay(volatile uint32_t* pu32_tick, uint32_t u32_itv);



#endif


