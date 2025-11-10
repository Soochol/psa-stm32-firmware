#include "lib_tim.h"


//use systick
static const uint32_t u32_MASK = 0xFFFFFFFF;
bool _b_Tim_Is_OVR(uint32_t u32_tick, uint32_t u32_ref, uint32_t u32_itv){
	uint32_t u32_std;
	if((u32_tick) < (u32_ref))	{u32_std = u32_MASK - (u32_ref) + u32_tick;}
	else						{u32_std = u32_tick - (u32_ref);}

	if(u32_std >= (u32_itv))	{return true;}
	else						{return false;}
}

bool _b_Tim_Is_OVR_withERR(uint32_t u32_tick, uint32_t u32_ref, uint32_t u32_itv, uint32_t u32_err){
	uint32_t u32_std;
	if((u32_tick) < (u32_ref))	{u32_std = u32_MASK - (u32_ref) + u32_tick;}
	else						{u32_std = u32_tick - (u32_ref);}

	if((u32_std >= (u32_itv - (u32_err))) || (u32_std <= (u32_itv + u32_err))){
		return true;
	}
	else{
		return false;
	}
}

uint32_t _u32_Tim_Get_ITV(uint32_t u32_tick, uint32_t u32_ref){
	if((u32_tick) < (u32_ref)){
		uint32_t u32_ovf = u32_MASK - (u32_ref);
		return (u32_tick + u32_ovf);
	}
	else{
		return (u32_tick - (u32_ref));
	}
}

void _v_Tim_Delay(volatile uint32_t* pu32_tick, uint32_t u32_itv){
	//tick keep move...
	uint32_t u32_ref = *pu32_tick;
	while(!_b_Tim_Is_OVR(*pu32_tick, u32_ref, u32_itv));
}



















