#ifndef __JH_TIM_H
#define __JH_TIM_H

#include "main.h"
#include "lib_def.h"
#include "lib_tim.h"


/****************************************/
//				SYSTICK					//
//	1ms timer							//
/****************************************/
uint32_t u32_Tim_1msGet();
void v_Tim_1s_Test();

void DWT_Init();
void delay_us(uint32_t us);
void v_1Cycle_Time();
/****************************************/
//				TIM2					//
//	CH1	: PWM (actuator heater)			//
//	CH2	: PWM (blow fan)				//
//	CH3	: PWM (heat pad)				//
//	CH4 : PWM (cool fan)				//
/****************************************/
void v_TIM2_Ch1_Out(uint16_t u16_pwm);
void v_TIM2_Ch2_Out(uint16_t u16_pwm);
void v_TIM2_Ch3_Out(uint16_t u16_pwm);
void v_TIM2_Ch4_Out(uint16_t u16_pwm);

/****************************************/
//				TIM4					//
//	CH2	: PWM (Digital RGB)				//
/****************************************/
void v_Tim4_Ch2_Out(uint16_t* pu16_pwmArr, uint16_t u16_cnt);




void v_Tim_Init();
void v_Tim_Handler();




#endif
