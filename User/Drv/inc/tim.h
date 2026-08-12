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
//		SD BLOCKING MEASUREMENT			//
/****************************************/
// Longest single SD access that held the main loop, in microseconds. Reported
// with the scenario 8 measurement to settle the SD_TIMEOUT_* values.
// Log-spaced buckets, upper edges in ms: 1, 2, 5, 10, 50, 100, 500, then the
// rest. A single maximum cannot separate a normal tail from a one-off, and the
// scenario 8 session has to answer both.
#define SD_BLOCK_BUCKETS	8

void     v_Tim_SD_Block_Init(void);
uint32_t u32_Tim_SD_Block_MaxUs(void);
uint32_t u32_Tim_SD_Block_Hist(uint8_t u8_bucket);
uint32_t u32_Tim_SD_Block_Count(void);
void     v_Tim_SD_Block_Clear(void);
/****************************************/
//				TIM2					//
//	CH1	: PWM (actuator heater)			//
//	CH2	: PWM (blow fan)				//
//	CH3	: PWM (heat pad)				//
//	CH4 : PWM (cool fan)				//
/****************************************/
#define TIM2_ARR_MAX	1000

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
