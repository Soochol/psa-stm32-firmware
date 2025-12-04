#include "adc.h"
#include "tim.h"
#include "lib_log.h"

/*
 * ADC1
 * - CH3	: ACT_COOLFAN
 * - CH10	: ACT_HEATER
 * - CH11	: VBAT
 *
 * ADC3
 * - CH0	: RANK1	- FAN
 * - CH1	: RANK2 - PAD
 * - CH18	: RANK5 - VINT
 */
//extern
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;

ADC_HandleTypeDef* p_adc1 = &hadc1;
ADC_HandleTypeDef* p_adc3 = &hadc3;

//#define ADC_VREF_FIXED	(3.3f)

//1ms or 10ms
#define ADC_ITV_MS		(1)

#define ADC_CPLT_ADC1	(0x01)
#define ADC_CPLT_ADC3	(0x02)
#define ADC_PROC_RDY	(ADC_CPLT_ADC1 | ADC_CPLT_ADC3)

#define ADC1_CH_CNT		3
#define ADC3_CH_CNT		3
#define ADC_CH_CNT		(ADC1_CH_CNT + ADC3_CH_CNT)
#define ADC_SAMP_CNT	(100)

typedef enum {
	ADC_POS_COOLFAN=0,
	ADC_POS_HEATER,
	ADC_POS_BATTERY,
	ADC_POS_FAN,
	ADC_POS_HEATPAD,
	ADC_POS_REFVOLT,
} ADC_POS_t;
//function
static void v_ADC_Proc();
static void v_ADC_Test();


#define ADC1_RAW_SIZE	(4)
#define ADC3_RAW_SIZE	(4)


static uint16_t u16_adc1_raw[ADC1_RAW_SIZE] __attribute__((section(".my_nocache_section"))); // 32-Byte aligned for cache maintenance
static uint16_t u16_adc3_raw[ADC3_RAW_SIZE] __attribute__((section(".my_nocache_section"))); // 32-Byte aligned for cache maintenances


static const uint16_t* pu16_refVoltCal = (uint16_t*)0x1FF1E860;
static const uint16_t u16_resol = 4095;	//12bit resolution

/*******************************************/
//	RAW data order
//	ADC1_CH3	: cool fan
//	ADC3_CH0	: fan
//	ADC3_CH1	: heat pad
//	ADC3_CH10	: heater
//	ADC3_CH11	: battery
//	ADC3_CH18	: reference volt
/*******************************************/
static uint16_t u16_adcRaw[ADC_CH_CNT][ADC_SAMP_CNT];
static uint16_t u16_adcAvg[ADC_CH_CNT];

static float f_refVolt;
static uint16_t adcCpltMask;

// Error state tracking for ADC DMA operations
static volatile uint8_t u8_adc1_dma_err = 0;
static volatile uint8_t u8_adc3_dma_err = 0;




/*
 * brief	: adc conversion complete
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	if(hadc->Instance == p_adc1->Instance){
		//CH 3 EA
		adcCpltMask |= ADC_CPLT_ADC1;
	}
	else if(hadc->Instance == p_adc3->Instance){
		//CH 3 EA
		adcCpltMask |= ADC_CPLT_ADC3;
	}
}



int adc_init_failed;
/*
 * brief	: adc driver initialize
 * date
 * - create	: 25.04.15
 * - modify	: 25.04.16
 */
void v_ADC_Init(){
	HAL_StatusTypeDef stat = HAL_OK;
	//ADC1 calibration
	stat = HAL_ADC_Stop(p_adc1);
	if(stat != HAL_OK){adc_init_failed++;}
	stat = HAL_ADCEx_Calibration_Start(p_adc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	if(stat != HAL_OK){adc_init_failed++;}
	//ADC3 calibration
	stat = HAL_ADC_Stop(p_adc3);
	if(stat != HAL_OK){adc_init_failed++;}
	stat = HAL_ADCEx_Calibration_Start(p_adc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	if(stat != HAL_OK){adc_init_failed++;}


	stat = HAL_ADC_Start_DMA(p_adc1, (uint32_t*)u16_adc1_raw, ADC1_CH_CNT);
	if(stat != HAL_OK){adc_init_failed++;}
	stat = HAL_ADC_Start_DMA(p_adc3, (uint32_t*)u16_adc3_raw, ADC3_CH_CNT);
	if(stat != HAL_OK){adc_init_failed++;}
}


/*
 * brief	: adc moving average process
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
static void v_ADC_Proc(){
	static uint16_t bufCnt;
	static uint16_t last[ADC_CH_CNT];
	static uint32_t sum[ADC_CH_CNT];
	static uint16_t rawIn;
	if(bufCnt < ADC_SAMP_CNT){++bufCnt;}
	for(uint16_t ch=0; ch<ADC_CH_CNT; ++ch){
		last[ch] = u16_adcRaw[ch][rawIn];
		//raw copy
		if(ch < ADC1_CH_CNT){
			u16_adcRaw[ch][rawIn] = u16_adc1_raw[ch];
		}
		else{
			u16_adcRaw[ch][rawIn] = u16_adc3_raw[ch - ADC1_CH_CNT];
		}
		//sum
		sum[ch] += u16_adcRaw[ch][rawIn];
		sum[ch] -= last[ch];
		//avg
		u16_adcAvg[ch] = sum[ch] / bufCnt;
	}

	if(rawIn < ADC_SAMP_CNT - 1){++rawIn;}
	else						{rawIn = 0;}
}

uint16_t u16_adc1_dr;
uint16_t u16_adc3_dr[8];
/*
 * brief	: adc data processing
 * date
 * - create	: 25.04.16
 * - modify	: 25.04.30
 */
void v_ADC_Handler(){
	//ADC1 and ADC3 complete
	static uint32_t timRef;
	//if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, ADC_ITV_MS)){
	if(adcCpltMask == ADC_PROC_RDY && _b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, ADC_ITV_MS)){
		timRef = u32_Tim_1msGet();
		adcCpltMask = 0;
		//process
		v_ADC_Proc();
		//reference internal volt refresh
		f_refVolt = f_ADC_Get_RefVolt();
		//conversion start
		v_ADC_Test();

		// HIGH: Check return value to detect DMA start failures
		if(HAL_ADC_Start_DMA(p_adc1, (uint32_t*)u16_adc1_raw, ADC1_CH_CNT) != HAL_OK){
			u8_adc1_dma_err++;
		}
		if(HAL_ADC_Start_DMA(p_adc3, (uint32_t*)u16_adc3_raw, ADC3_CH_CNT) != HAL_OK){
			u8_adc3_dma_err++;
		}
	}
}

float f_curr_coolfan;
float f_curr_fan;
float f_curr_heatpad;
float f_curr_heater;
float f_vbat;
static void v_ADC_Test(){
	f_curr_coolfan = f_ADC_Get_CoolFanCurr();
	f_curr_fan = f_ADC_Get_FanCurr();
	f_curr_heatpad = f_ADC_Get_HeatPadCurr();
	f_curr_heater = f_ADC_Get_HeaterCurr();
	f_vbat = f_ADC_Get_BatVolt();

}


/*******************************************/
//	COOLFAN
//	Rs 	: 0.1 ohm
/*******************************************/

/*
 * brief	: actuator cooler fan current
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
float f_ADC_Get_CoolFanCurr(){
	float V = (float)(u16_adcAvg[ADC_POS_COOLFAN] * f_refVolt) / u16_resol;
	float I = V / 0.1;
	return I;
}

/*******************************************/
//	FAN
//	Rs 	: 0.1 ohm
/*******************************************/

/*
 * brief	: fan current
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
float f_ADC_Get_FanCurr(){
	float V = (float)(u16_adcAvg[ADC_POS_FAN] * f_refVolt) / u16_resol;
	float I = V / 0.1;
	return I;
}

/*******************************************/
//	HEAT PAD
//	Rs	: 0.1 ohm
/*******************************************/

/*
 * brief	: heat pad current
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
float f_ADC_Get_HeatPadCurr(){
	float V = (float)(u16_adcAvg[ADC_POS_HEATPAD] * f_refVolt) / u16_resol;
	float I = V / 0.1;
	return I;
}

/*******************************************/
//	HEATER
//	Rs		: 1 mohm
//	Gain	: 50 (RL : 49.9K)
//	40A -> 2V
/*******************************************/
float heaterVolt;
/*
 * brief	: actuator heater current
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
float f_ADC_Get_HeaterCurr(){
	float V = (float)(u16_adcAvg[ADC_POS_HEATER] * f_refVolt) / u16_resol;
	float I = V / (0.05);	//0.001 * 50
	//float I = V * 50;
	heaterVolt = V;

	return I;
}



/*******************************************/
//	BATTERY
//	Divider	: 1/16
/*******************************************/

/*
 * brief	: battery volt
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
float f_ADC_Get_BatVolt(){
	float V = (float)(u16_adcAvg[ADC_POS_BATTERY] * f_refVolt) / u16_resol;
	return (V * 16);
}


/*******************************************/
//	REFERENCE VOLT
/*******************************************/

/*
 * brief	: reference interval volt
 * date
 * - create	: 25.04.30
 * - modify	: -
 */
float f_ADC_Get_RefVolt(){
#ifdef ADC_VREF_FIXED
	float V = ADC_VREF_FIXED;
#else
	float V = (float)(3.3 * *pu16_refVoltCal) / u16_adcAvg[ADC_POS_REFVOLT];
#endif
	return V;
}









