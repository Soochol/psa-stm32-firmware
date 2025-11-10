#include "lib_mode.h"
#include "string.h"


void v_Mode(_x_modeCFG_t* px_cfg, void* param){
	if(px_cfg->cr.bit.b1_upd){
		//initial
		if(px_cfg->cr.bit.b1_on){

		}
		else if(px_cfg->cr.bit.b1_off){





			_v_Mode_Next(px_cfg);
		}
		else if(px_cfg->cr.bit.b1_stop){



			//lock disable

		}
	}

	//polling
	if(px_cfg->cr.bit.b1_on){

	}
	else if(px_cfg->cr.bit.b1_off){

	}
	else if(px_cfg->cr.bit.b1_stop){

	}
}



void _v_Mode_Set_Next(_x_modeCFG_t* px_cfg){
	px_cfg->fn.v_curr = px_cfg->fn.v_next;
}

void _v_Mode_Next(_x_modeCFG_t* px_cfg){
	px_cfg->cr.u8 = 0;
	if(px_cfg->fn.v_next != NULL){
		px_cfg->fn.v_curr = px_cfg->fn.v_next;
		px_cfg->cr.u8 = _modeCR_UPD_MASK | _modeCR_ON_MASK;
	}
	else{
		px_cfg->fn.v_curr = NULL;
	}
}

bool _b_Mode_Run(_x_modeCFG_t* px_cfg, void* pv_param){
	if(px_cfg->fn.v_curr != NULL){
		px_cfg->fn.v_curr(pv_param);
		return true;
	}
	else{
		//mode empty or error -> mode off
		//system reset
		return false;
	}
}

//example

























