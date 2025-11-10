#include "main.h"
#include "key.h"
#include "tim.h"
#include "mode.h"
#include "stdio.h"
#include "string.h"
//static _x_keyTIM_t key_

void v_Key_BlowFan_Handler();
void v_Key_ForceToggle_Handler();
void v_Key_Power_Handler();

void v_Key_Handler(){
	v_Key_BlowFan_Handler();
	v_Key_ForceToggle_Handler();
	v_Key_Power_Handler();
}


void v_Key_ForceToggle_Handler(){
	static _x_keyTIM_t time = {
		.u32_ref_pressed=0,
		.u32_ref_released=0,
		.u32_val_pressed = 10,
		.u32_val_released = 5,
		.u32_tick = u32_Tim_1msGet,
	};
	static _x_keyACT_t act;
	if(_b_Key_Get_Act(&act, _e_Key_Get(&time, !CHK_BIT(DI_CTRL_SW2_GPIO_Port->IDR, DI_CTRL_SW2_Pin)))){
		if(act.reg.bit.b1_upd){
			if(act.reg.bit.b1_short){
				HAL_GPIO_WritePin(DO_EXT_IO2_GPIO_Port, DO_EXT_IO2_Pin, GPIO_PIN_RESET);
				e_modeID_t id = e_Mode_Get_CurrID();
				v_Mode_Enable_ToggleSW();
				if(id == modeFORCE_UP || id == modeFORCE_ON){
					v_Mode_SetNext(modeFORCE_DOWN);
				}
				else{
					v_Mode_SetNext(modeFORCE_UP);
				}
			}
			else{
				HAL_GPIO_WritePin(DO_EXT_IO2_GPIO_Port, DO_EXT_IO2_Pin, GPIO_PIN_SET);
			}
			act.reg.u8 = 0;
		}
	}
}


void v_Key_BlowFan_Handler(){
	static _x_keyTIM_t time = {
		.u32_ref_pressed=0,
		.u32_ref_released=0,
		.u32_val_pressed = 10,
		.u32_val_released = 5,
		.u32_tick = u32_Tim_1msGet,
	};
	static _x_keyACT_t act;
	if(_b_Key_Get_Act(&act, _e_Key_Get(&time, !CHK_BIT(DI_CTRL_SW1_GPIO_Port->IDR, DI_CTRL_SW1_Pin)))){
		if(act.reg.bit.b1_upd){
			if(act.reg.bit.b1_short){
				HAL_GPIO_WritePin(DO_EXT_IO1_GPIO_Port, DO_EXT_IO1_Pin, GPIO_PIN_RESET);

				uint16_t now = u16_Mode_Get_BlowFan_Now();
				uint16_t max = u16_Mode_Get_BlowFan_Max();

				if(now < max)	{now++;}
				else			{now = 0;}
				v_Mode_Set_BlowFan_Now(now);
			}
			else{
				HAL_GPIO_WritePin(DO_EXT_IO1_GPIO_Port, DO_EXT_IO1_Pin, GPIO_PIN_SET);
			}
			act.reg.u8 = 0;
		}
	}
}



uint32_t u32_bkup_cnt;
uint32_t u32_long_key;
static _x_keyACT_t x_keyPWR;
static _e_keyREAD_t e_keyReadPWR;
void v_Key_Power_Init(){
	memset(&x_keyPWR, 0, sizeof(_x_keyACT_t));
}

_e_keyREAD_t e_Key_Read_PWR(){
	return e_keyReadPWR;
}

void v_Key_Power_Handler(){
	static _x_keyTIM_t time = {
		.u32_ref_pressed=0,
		.u32_ref_released=0,
		.u32_val_pressed = 10,
		.u32_val_released = 10,
		.u32_val_long_pressed = 1500,
		.u32_tick = u32_Tim_1msGet,
	};
	_x_keyACT_t* p_act = &x_keyPWR;
	e_keyReadPWR = _e_Key_Get_Long(&time, !CHK_BIT(EXTI14_PWR_GPIO_Port->IDR, EXTI14_PWR_Pin));
	if(_b_Key_Get_Act(p_act, e_keyReadPWR)){
		if(p_act->reg.bit.b1_upd){
			e_modeID_t id = e_Mode_Get_CurrID();
			if(p_act->reg.bit.b1_rst){
				if(!p_act->reg.bit.b1_long){
					//short
					if(id == modeOFF){
						v_Mode_SetNext(modeOFF);
					}
				}
				p_act->reg.u8 = 0;
				HAL_GPIO_WritePin(DO_EXT_IO3_GPIO_Port, DO_EXT_IO3_Pin, GPIO_PIN_SET);
			}
			else{
				if(p_act->reg.bit.b1_long){
					//long
					if(id > modeBOOTING){
						if(id != modeOFF){
							//power down
							v_Mode_SetNext(modeOFF);
						}
						else{
							//reset
							v_Mode_SetNext(modeWAKE_UP);
						}
					}
					HAL_GPIO_WritePin(DO_EXT_IO3_GPIO_Port, DO_EXT_IO3_Pin, GPIO_PIN_RESET);
				}
			}
		}
	}
}


uint32_t u32_exti_cnt;


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == EXTI14_PWR_Pin){
		u32_exti_cnt++;
	}
}




