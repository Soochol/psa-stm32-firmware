#include "lib_dim.h"
#include "lib_tim.h"



void _v_Dim_CFG(_x_DIM_CFG_t* px_cfg, uint32_t* pu32_tick, uint32_t u32_itv, uint32_t u32_keep,\
			   uint32_t u32_max, uint32_t u32_min, _e_DIM_t e_dir){
	//time
	px_cfg->tim.pu32_tick = pu32_tick;
	px_cfg->tim.u32_itv = u32_itv;
	px_cfg->tim.u32_keep = u32_keep / u32_itv;
	px_cfg->tim.u32_ref_itv = *pu32_tick;
	px_cfg->tim.u32_ref_keep = 0;
	//dim
	px_cfg->dim.u32_max = u32_max;
	px_cfg->dim.u32_min = u32_min;
	px_cfg->dim.u32_idx = 0;
	px_cfg->dim.e_val_dir = e_dir;
	if(e_dir != _dimUPDOWN){px_cfg->dim.e_now_dir = e_dir;}
	else					{px_cfg->dim.e_now_dir = _dimUP;}
}

uint32_t _u32_Dim_Get_IDX(_x_DIM_CFG_t* px_cfg){
	if(_b_Tim_Is_OVR(*px_cfg->tim.pu32_tick, px_cfg->tim.u32_ref_itv, px_cfg->tim.u32_itv)){
		px_cfg->tim.u32_ref_itv = *px_cfg->tim.pu32_tick;
		//keep default
		if(px_cfg->tim.u32_ref_keep){px_cfg->tim.u32_ref_keep--;}

		//DIM-UP
		if(!px_cfg->tim.u32_ref_keep && px_cfg->dim.e_now_dir == _dimUP){
			if(px_cfg->dim.u32_idx < px_cfg->dim.u32_max){
				px_cfg->dim.u32_idx++;
			}
			else if(px_cfg->dim.e_val_dir & _dimDOWN){
				px_cfg->dim.e_now_dir = _dimDOWN;	//initial copy... need ?
				px_cfg->tim.u32_ref_keep = px_cfg->tim.u32_keep;
			}
		}
		//DIM-DOWN
		else if(!px_cfg->tim.u32_ref_keep && px_cfg->dim.e_now_dir == _dimDOWN){
			if(px_cfg->dim.u32_idx > px_cfg->dim.u32_min){
				px_cfg->dim.u32_idx--;
			}
			else if(px_cfg->dim.e_val_dir & _dimUP){
				px_cfg->dim.e_now_dir = _dimUP;
				px_cfg->tim.u32_ref_keep = px_cfg->tim.u32_keep;
			}
		}
	}
	return px_cfg->dim.u32_idx;
}

bool _b_Dim_Is_On(_x_DIM_CFG_t* px_cfg, uint32_t u32_idx){
	if(!_b_Tim_Is_OVR(*px_cfg->tim.pu32_tick, px_cfg->tim.u32_ref_itv, u32_idx)){
		return true;
	}
	else{
		return false;
	}
}

void v_LED_Output(){

}








