#include "lib_buzz.h"
#include "lib_tim.h"

void _v_Buzz_Init(_x_buzzCFG_t* px_cfg, uint32_t* pu32_tick, void(*v_Play)(bool, void*), bool(*b_IsDone)(void)){
	px_cfg->tim.pu32_tick = pu32_tick;
	px_cfg->fn.v_Play = v_Play;
	px_cfg->fn.b_IsDone = b_IsDone;
	px_cfg->e_stat = _buzzIDLE;
}

bool _b_Buzz_Set_CFG(_x_buzzCFG_t* px_cfg, void* pv_play_param, uint8_t u8_play_cnt, uint32_t u32_delay){
	if(px_cfg->e_stat == _buzzIDLE){
		px_cfg->pv_play_param = pv_play_param;
		px_cfg->u8_play_cnt = u8_play_cnt;
		px_cfg->tim.u32_delay = u32_delay;

		if(px_cfg->u8_play_cnt){
			px_cfg->u8_play_cnt--;
			px_cfg->fn.v_Play(true, pv_play_param);
			px_cfg->e_stat = _buzzPLAY;
		}
		return true;
	}
	else{
		return false;
	}
}

void _v_Buzz_Stop(_x_buzzCFG_t* px_cfg){
	px_cfg->e_stat = _buzzSTOP;
}

static void _v_Buzz_Ctrl(_x_buzzCFG_t* px_cfg){
	if((px_cfg->e_stat != _buzzIDLE) && (px_cfg->e_stat != _buzzPLAY)){
		if(px_cfg->e_stat == _buzzSTOP){
			px_cfg->fn.v_Play(false, px_cfg->pv_play_param);
			px_cfg->e_stat = _buzzIDLE;
		}
		else{
			if(px_cfg->e_stat == _buzzDONE){
				if(px_cfg->tim.u32_delay){
					px_cfg->tim.u32_ref = *px_cfg->tim.pu32_tick;
					px_cfg->e_stat = _buzzDELAY;
				}
				else{
					px_cfg->e_stat = _buzzIDLE;
				}
			}
			if(px_cfg->e_stat == _buzzDELAY){
				if(_b_Tim_Is_OVR(*px_cfg->tim.pu32_tick, px_cfg->tim.u32_ref, px_cfg->tim.u32_delay)){
					px_cfg->e_stat = _buzzIDLE;
				}
			}
			if(px_cfg->e_stat == _buzzIDLE){
				if(px_cfg->u8_play_cnt){
					px_cfg->u8_play_cnt--;
					px_cfg->fn.v_Play(true, px_cfg->pv_play_param);
					px_cfg->e_stat = _buzzPLAY;
				}
			}
		}
	}
}

void _v_Buzz_Proc(_x_buzzCFG_t* px_cfg){
	if(px_cfg->fn.b_IsDone()){
		px_cfg->e_stat = _buzzDONE;
		px_cfg->fn.v_Play(false, px_cfg->pv_play_param);
	}
	_v_Buzz_Ctrl(px_cfg);
}

