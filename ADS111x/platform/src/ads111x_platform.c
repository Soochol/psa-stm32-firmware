#include "main.h"
#include "ads111x_platform.h"
#include "tim.h"
#include "i2c.h"
#include "adc.h"
#include "mode.h"

/*
 *
 * ADS1115 CONIFG
 * - OS		: 1
 * - MUX	: 100 (AIN0 single)
 * - PGA	: 001 (volt range : 4.096)
 * - MODE	: 1
 * - DR		: 100 (data rate)	: 128 SPS
 *
 */
#define ADS111X_CONFIG_VAL	(0xC283)

uint16_t u16_fsr_left, u16_fsr_right;

static bool b_fsr_rdDone;
static bool b_fsr_wrDone;

bool b_fsr_left, b_fsr_right;

uint16_t u16_fsr_in_left, u16_fsr_in_right;
uint16_t u16_fsr_adc_left[8];
uint16_t u16_fsr_adc_right[8];
uint16_t u16_fsr_cnt_left, u16_fsr_cnt_right;


static e_COMM_STAT_t e_fsr_config;

//static uint16_t
static int i_toutAct_L, i_toutAct_R;
static uint32_t u32_toutRef_L, u32_toutRef_R;


static void v_FSR_Convert_Handler();

void v_ADS111X_RdDone(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_cnt){
	if(u8_addr == ADDR_FSR_LEFT){
		u16_fsr_left = (pu8_arr[0] << 8) & 0xFF00;
		u16_fsr_left |= pu8_arr[1] & 0x00FF;

		b_fsr_left = true;

		u16_fsr_adc_left[u16_fsr_in_left] = u16_fsr_left;
		if(u16_fsr_cnt_left < 8){u16_fsr_cnt_left++;}
		u16_fsr_in_left = (u16_fsr_in_left + 1) & 7;

		i_toutAct_L = 0;
	}
	else{
		u16_fsr_right = (pu8_arr[0] << 8) & 0xFF00;
		u16_fsr_right |= pu8_arr[1] & 0x00FF;

		b_fsr_right = true;

		u16_fsr_adc_right[u16_fsr_in_right] = u16_fsr_right;
		if(u16_fsr_cnt_right < 8){u16_fsr_cnt_right++;}
		u16_fsr_in_right = (u16_fsr_in_right + 1) & 7;

		i_toutAct_R = 0;
	}
	b_fsr_rdDone = true;
}


void v_ADS111X_WrDone(uint8_t u8_addr){
	if(u8_addr == ADDR_FSR_LEFT){i_toutAct_L = 0;}
	else						{i_toutAct_R = 0;}
	b_fsr_wrDone = true;
}



int i_FSR_Write(uint8_t u8_addr, uint16_t u16_memAddr, uint8_t* pu8, uint16_t u16_cnt){
	if(u8_addr == ADDR_FSR_LEFT){
		i_toutAct_L = 1;
		u32_toutRef_L = u32_Tim_1msGet();
	}
	else{
		i_toutAct_R = 1;
		u32_toutRef_R = u32_Tim_1msGet();
	}
	return i_I2C2_Write(u8_addr, u16_memAddr, pu8, u16_cnt);
}

int i_FSR_Read(uint8_t u8_addr, uint16_t u16_memAddr, uint16_t u16_cnt){
	if(u8_addr == ADDR_FSR_LEFT){
		i_toutAct_L = 1;
		u32_toutRef_L = u32_Tim_1msGet();
	}
	else{
		i_toutAct_R = 1;
		u32_toutRef_R = u32_Tim_1msGet();
	}
	return i_I2C2_Read(u8_addr, u16_memAddr, u16_cnt);
}


//place in main
void v_FSR_Tout_Handler(){
	//if left active
	if(i_toutAct_L && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef_L, 2000)){
		//timeout
		i_toutAct_L = 0;
		e_fsr_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_FSR);
		v_Mode_SetNext(modeERROR);
	}

	//if right active
	if(i_toutAct_R && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef_R, 2000)){
		//timeout
		i_toutAct_R = 0;
		e_fsr_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_FSR);
		v_Mode_SetNext(modeERROR);
	}
}


void v_FSR_Deinit(){
	e_fsr_config = COMM_STAT_READY;
}

e_COMM_STAT_t e_FSR_Ready(){
	static uint32_t timRef, timItv;
	static uint16_t mask;
	if(e_fsr_config == COMM_STAT_READY){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
			timRef = u32_Tim_1msGet();

			uint8_t arr[4] = {0,};
			arr[0] = (uint8_t)(ADS111X_CONFIG_VAL >> 8);
			arr[1] = (uint8_t)ADS111X_CONFIG_VAL;

			if(!mask){
				if(i_FSR_Write(ADDR_FSR_LEFT, ADS111X_REG_CONFIG, arr, 2) == COMM_STAT_OK){
					mask  = 1;
				}
				else{
					timItv = 10;
					return false;
				}
			}
			if(i_FSR_Write(ADDR_FSR_RIGHT, ADS111X_REG_CONFIG, arr, 2) == COMM_STAT_OK){
				mask = 0;
				e_fsr_config = COMM_STAT_DONE;
			}
			else{
				timItv = 10;
			}
		}
	}
	return e_fsr_config;
}





void v_FSR_Data_Handler(){
	static uint32_t timRef, timItv;
	static uint16_t mask;
	if(e_fsr_config != COMM_STAT_DONE){return;}
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
		timRef = u32_Tim_1msGet();

		if(!mask){
			if(i_FSR_Read(ADDR_FSR_LEFT, ADS111X_REG_CONVERT, 2) == COMM_STAT_OK){
				mask  = 1;
			}
			else{
				timItv = 10;
				return;
			}
		}
		if(i_FSR_Read(ADDR_FSR_RIGHT, ADS111X_REG_CONVERT, 2) == COMM_STAT_OK){
			mask  = 0;
			timItv = 100;
		}
		else{
			timItv = 10;
			return;
		}
	}
	//v_FSR_Convert_Handler();
}

#define FSR406_CORRECT	0.0008f





const float A_COEFF = 1.1e-4;        // FSR406 force curve coefficient a
const float B_COEFF = -0.95;         // FSR406 force curve exponent b
const float G_ACC   = 9.81;          // Gravity acceleration (m/s²)

#define ADS111X_PGA		4.096f
#define ADS111X_RESOL	(32768.0)

float f_FSR_Convert_Gram(uint16_t u16_adc){
	float V_in = 3.3;
	//float R_fixed = 3000.0;	//3kOhm
	float voltage = (u16_adc * ADS111X_PGA) / ADS111X_RESOL;

	if(voltage <= 0.01 || voltage >= V_in){return 0.0;}

#if 0
	float R_fsr = R_fixed * ((V_in / voltage) - 1.0);	//unit : ohm
	float force = A_COEFF * pow(R_fsr, B_COEFF);
	float mass_g = (force / G_ACC) * 1000.0;
	return mass_g;
#endif

#if 0

	if(R_fsr <= 60000){
		force = (1000000.0 / R_fsr) / 80.0;
	}
	else{
		force = ((1000000.0 / R_fsr) - 1000.0) / 30.0;
	}
	float mass_gram = force / 9.81 * 1000.0;	//gram
#endif

	//1.00.04
	float a = 25.0;
	float b = 2.5;
	float mass_gram = a * pow(voltage, b);

	return mass_gram;
}

float fsrGram_Left, fsrGram_Right;
float fsrVolt_Left, fsrVolt_Right;
uint16_t fsrADC_Left, fsrADC_Right;
__attribute__((unused)) static void v_FSR_Convert_Handler(){
	if(b_fsr_left){
		b_fsr_left = false;
		uint32_t sum=0;
		for(int i=0; i<u16_fsr_cnt_left; i++){
			sum += u16_fsr_adc_left[i];
		}
		uint16_t avg = sum / u16_fsr_cnt_left;
		fsrGram_Left = f_FSR_Convert_Gram(avg);

		fsrADC_Left = avg;
		fsrVolt_Left = (avg * ADS111X_PGA) / ADS111X_RESOL;
	}
	if(b_fsr_right){
		b_fsr_right = false;
		uint32_t sum=0;
		for(int i=0; i<u16_fsr_cnt_right; i++){
			sum += u16_fsr_adc_right[i];
		}
		uint16_t avg = sum / u16_fsr_cnt_right;
		fsrGram_Right = f_FSR_Convert_Gram(avg);

		fsrADC_Right = avg;
		fsrVolt_Right = (avg * ADS111X_PGA) / ADS111X_RESOL;
	}
}


float f_FSR_Get_Left(){
	return fsrGram_Left;
}

float f_FSR_Get_Right(){
	return fsrGram_Right;
}


uint16_t u16_FSR_Get_Left(){
	return u16_fsr_left;
}

uint16_t u16_FSR_Get_Right(){
	return u16_fsr_right;
}

