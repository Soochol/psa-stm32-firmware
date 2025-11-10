#include "lib_key.h"
#include "lib_tim.h"


//e_

//PRESSED or RELEASED
_e_keyREAD_t _e_Key_Get(_x_keyTIM_t* px_tim, uint32_t u32_key){
	if(u32_key){
		//pressed
		if(_b_Tim_Is_OVR(px_tim->u32_tick(), px_tim->u32_ref_pressed, px_tim->u32_val_pressed)){
			px_tim->u32_ref_released = px_tim->u32_tick();
			return _keyPRESSED;
		}
		else{
			return _keyRELEASED;
		}
	}
	else{
		//released
		if(_b_Tim_Is_OVR(px_tim->u32_tick(), px_tim->u32_ref_released, px_tim->u32_val_released)){
			px_tim->u32_ref_pressed = px_tim->u32_tick();
			return _keyRELEASED;
		}
		else{
			return _keyPRESSED;
		}
	}
}

_e_keyREAD_t _e_Key_Get_Long(_x_keyTIM_t* px_tim, uint32_t u32_key){
	if(_e_Key_Get(px_tim, u32_key)){
		if(_b_Tim_Is_OVR(px_tim->u32_tick(), px_tim->u32_ref_pressed, px_tim->u32_val_long_pressed)){
			return _keyLONG_PRESSED;
		}
		else{
			return _keyPRESSED;
		}
	}
	else{
		return _keyRELEASED;
	}
}


//BLINK
_e_keyBLINK_t _e_Key_Get_Blink(_x_keyBLINK_CFG_t* px_cfg, uint32_t u32_itv, uint32_t u32_err, uint32_t u32_key){
	_e_keyBLINK_t e_now;
	if(u32_key)	{e_now = _keyHIGH;}
	else		{e_now = _keyLOW;}

	if((px_cfg->e_ref != e_now) || _b_Tim_Is_OVR_withERR(*px_cfg->tim.pu32_tick, px_cfg->tim.u32_ref, u32_itv, u32_err)){
		px_cfg->e_ref = e_now;

		if(e_now == _keyLOW){
			if((px_cfg->u8_low == 0) && (px_cfg->u8_high == 1))	{px_cfg->e_val = _keyBLINK;}
			else												{px_cfg->e_val = _keyLOW;}

			if(px_cfg->u8_low < 2)	{px_cfg->u8_low++;}
			px_cfg->u8_high = 0;
		}
		else{
			if((px_cfg->u8_high == 0) && (px_cfg->u8_low == 1))	{px_cfg->e_val = _keyBLINK;}
			else												{px_cfg->e_val = _keyHIGH;}

			if(px_cfg->u8_high < 2)	{px_cfg->u8_high++;}
			px_cfg->u8_low = 0;
		}

		if(e_now == _keyBLINK)	{px_cfg->tim.u32_ref = *px_cfg->tim.pu32_tick;}
		else					{px_cfg->tim.u32_ref += u32_itv;}
	}
	return px_cfg->e_val;
}


//SET
bool _b_Key_Get_Act(_x_keyACT_t* px_act, _e_keyREAD_t e_key){
	if(px_act->e_ref != e_key){
		px_act->e_ref = e_key;

		px_act->reg.bit.b1_upd = 1;
		px_act->reg.u8 |= 1 << e_key;
		return true;
	}
	else{
		return false;
	}
}


#if 0
//example
_x_keyACT_t x_Get_SW(){
	static uint32_t u32_1ms;
	uint32_t key_val = 1;
	static _e_keyREAD_t e_ref;
	static _x_keyTIM_t x_time = {
		.u32_ref_pressed = 0,
		.u32_ref_released = 0,
		.u32_val_pressed = 50,
		.u32_val_released = 5,
		.u32_val_long_pressed = 2000,
		.pu32_tick = &u32_1ms,
	};
	return _x_Key_Get_Act(&e_ref, _e_Key_Get_Long(&x_time, key_val));
}
#endif













