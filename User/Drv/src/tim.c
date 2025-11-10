#include "tim.h"
#include "lib_tim.h"
#include "uart.h"
#include "sk6812_platform.h"
//extern
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

extern TIM_HandleTypeDef htim7;

//static
static TIM_HandleTypeDef* p_tim2 = &htim2;
static TIM_HandleTypeDef* p_tim4 = &htim4;


static TIM_HandleTypeDef* p_tim7 = &htim7;

/****************************************/
//				SYSTICK					//
//	1ms timer							//
/****************************************/

/*
 * brief	: get 1ms tick counts
 * date
 * - create	: 25.04.16
 * - modify	: -
 */
uint32_t u32_Tim_1msGet(){
	return HAL_GetTick();
}

uint32_t u32_tick1s;
void v_Tim_1s_Test(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 1000)){
		timRef = u32_Tim_1msGet();
		++u32_tick1s;
		//v_printf_poll("tim : %d\n", u32_tick1s);
	}
	v_1Cycle_Time();
}





bool isDWTEnable(){
	return (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);
}

void DWT_Init(){
	if(isDWTEnable() == false){
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;	//DWT used enable
		DWT->CYCCNT = 0;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}

void delay_us(uint32_t us){
	uint32_t start = DWT->CYCCNT;
	uint32_t ticks = us * (SystemCoreClock / 1000000);

	while((DWT->CYCCNT - start) < ticks);
}

uint32_t one_cycle;
uint32_t one_cycle_time;
void v_1Cycle_Time(){
	static uint32_t start;

	uint32_t end = DWT->CYCCNT;
	if(end >= start){
		one_cycle = end - start;
	}
	else{
		one_cycle = 0xFFFFFFFF - start + end;
	}
	//one_cycle_time = one_cycle *

	start = DWT->CYCCNT;
}


/****************************************/
//				TIM1					//
//	Update event - TRGO					//
//	clock : 192,000,000 Hz				//
//	prescaler : 1						//
//	counts	: 8000						//
//	overflow : 24,000 Hz				//
/****************************************/
void v_TIM1_Init(){

}




/****************************************/
//				TIM2					//
//	CH1	: PWM (actuator heater)			//
//	CH2	: PWM (fan)						//
//	CH3	: PWM (heat pad)				//
//	CH4 : PWM (actuator cool fan)		//
//	clock : 192,000,000 Hz				//
//	prescaler : 1920					//
//	counts	: 1000						//
//	overflow : 100 Hz					//
/****************************************/


/*
 * brief	: TIM2 CH1 pwm output
 * date
 * - create	: 25.07.25
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch1_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev = 0xFFFF;

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_1, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_1);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_1);
			on = false;
		}
	}
}


/*
 * brief	: TIM2 CH2 pwm output
 * date
 * - create	: 25.07.16
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch2_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev = 0xFFFF;

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_2, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_2);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_2);
			on = false;
		}
	}
}

/*
 * brief	: TIM2 CH3 pwm output
 * date
 * - create	: 25.04.16
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch3_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev = 0xFFFF;

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_3, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_3);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_3);
			on = false;
		}
	}
}



/*
 * brief	: TIM2 CH4 pwm output
 * date
 * - create	: 25.04.16
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch4_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev;

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_4, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_4);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_4);
			on = false;
		}
	}
}


/****************************************/
//				TIM4					//
//	CH2	: PWM (Digital RGB)				//
//	clock : 192,000,000 Hz				//
//	prescaler : 1						//
//	counts	: 240						//
//	overflow : 800,000 Hz				//
/****************************************/
#include "lib_ringbuf.h"

#define TIM4_LED_SIZE	512
_RING_VAR_DEF(drgb, uint16_t, TIM4_LED_SIZE);

static bool b_tim4_ready;
uint16_t led_send_cnt;

int led_out;

#define TIM4_CACHE_ENABLE	0
#if TIM4_CACHE_ENABLE
ALIGN_32BYTES(static volatile uint16_t u16_tim4_pwm[TIM4_LED_SIZE] __attribute__((section(".my_d2_section")))); // 32-Byte aligned for cache maintenance
#else
static volatile uint16_t u16_tim4_pwm[TIM4_LED_SIZE] __attribute__((section(".my_nocache_section"))); // 32-Byte aligned for cache maintenance
#endif
/*
 * brief	: TIM4 CH2 pwm output
 * date
 * - create	: 25.04.16
 * - modify	: 25.04.29
 * param
 * - pu16_pwmArr	: led array
 * - u16_cnt		: array counts
 * note
 */
#if 0
void v_Tim4_Ch2_Out(uint16_t* pu16_pwmArr, uint16_t u16_cnt){
	if(b_tim4_ready == false){
		//__HAL_TIM_SET_COUNTER(p_tim4, 0);

		led_send_cnt = u16_cnt;
		for(int i=0; i<u16_cnt; i++){
			u16_tim4_pwm[i] = pu16_pwmArr[i];
		}
#if TIM4_CACHE_ENABLE
		SCB_CleanDCache_by_Addr((uint32_t*)&u16_tim4_pwm[0], sizeof(uint16_t) * TIM4_LED_SIZE);//after multiple calculation
#endif
		HAL_TIM_PWM_Start_DMA(p_tim4, TIM_CHANNEL_2, (uint32_t*)u16_tim4_pwm, u16_cnt);
		b_tim4_ready = true;
	}
	else{
		drgb->fn.v_PutArr(drgb, pu16_pwmArr, u16_cnt);
	}
}
#endif


void v_Tim4_Ch2_Out(uint16_t* pu16_pwmArr, uint16_t u16_cnt){
	if(b_tim4_ready == false){

		//HAL_TIM_PWM_Stop_DMA(p_tim4, TIM_CHANNEL_2);

		__HAL_TIM_CLEAR_FLAG(p_tim4, TIM_FLAG_CC2);
		__HAL_TIM_CLEAR_FLAG(p_tim4, TIM_FLAG_UPDATE);

		__HAL_TIM_DISABLE_OCxPRELOAD(p_tim4, TIM_CHANNEL_2);
		__HAL_TIM_SET_COMPARE(p_tim4, TIM_CHANNEL_2, 0);
		//__HAL_TIM_SET_COUNTER(p_tim4, 0);


		led_send_cnt = u16_cnt;
		for(int i=0; i<u16_cnt; i++){
			u16_tim4_pwm[i] = pu16_pwmArr[i];
		}
#if TIM4_CACHE_ENABLE
		SCB_CleanDCache_by_Addr((uint32_t*)&u16_tim4_pwm[0], sizeof(uint16_t) * TIM4_LED_SIZE);//after multiple calculation
#endif
		HAL_TIM_PWM_Start_DMA(p_tim4, TIM_CHANNEL_2, (uint32_t*)u16_tim4_pwm, u16_cnt + 1);	//cnt + 1 -> add low output
		__HAL_TIM_ENABLE_OCxPRELOAD(p_tim4, TIM_CHANNEL_2);
		b_tim4_ready = true;
	}
}






/****************************************/
//				TIM7					//
//	CH2	: PWM (Digital RGB)				//
//	clock : 192,000,000 Hz				//
//	prescaler : 1						//
//	counts	: 192						//
//	reload	: 100						//
//	overflow : 10,000 Hz				//
/****************************************/
uint32_t PCLK2;
void v_Tim_Init(){
	b_tim4_ready = false;
	HAL_TIM_Base_Start_IT(p_tim7);
	for(int i=0; i<sizeof(u16_tim4_pwm); i++){
		u16_tim4_pwm[0] = 0;
	}
	PCLK2 = HAL_RCC_GetPCLK2Freq();
}

void v_Tim_Handler(){

}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM7){
		v_RGB_Done_Handler();
	}
}



void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM4){
		b_tim4_ready = false;
		HAL_TIM_PWM_Stop_DMA(p_tim4, TIM_CHANNEL_2);
	}
}







