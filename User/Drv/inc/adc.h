#ifndef __JH_ADC_H
#define __JH_ADC_H

#include "main.h"
#include "lib_def.h"

/*******************************************/
//	ADC HARDWARE CONSTANTS
/*******************************************/
// ADC resolution and reference
#define ADC_RESOLUTION_12BIT		4095		// 12-bit ADC maximum value
#define ADC_VREF_VOLTAGE			3.3f		// ADC reference voltage
#define ADC_VREF_CAL_VOLTAGE		3.3f		// Reference voltage calibration constant

// Current sense resistor values (unit: ohm)
#define ADC_COOLFAN_SHUNT_RESISTANCE	0.1f	// Cool fan current shunt resistor
#define ADC_FAN_SHUNT_RESISTANCE		0.1f	// Fan current shunt resistor
#define ADC_HEATPAD_SHUNT_RESISTANCE	0.1f	// Heat pad current shunt resistor
#define ADC_HEATER_SENSE_GAIN			0.05f	// Heater current sense gain factor (0.001 * 50)

// Voltage divider ratios
#define ADC_BATTERY_DIV_RATIO		16			// Battery voltage divider ratio

void v_ADC_Init();
void v_ADC_Handler();



/*******************************************/
//	COOLFAN
/*******************************************/
float f_ADC_Get_CoolFanCurr();

/*******************************************/
//	FAN
/*******************************************/
float f_ADC_Get_FanCurr();

/*******************************************/
//	HEAT PAD
/*******************************************/
float f_ADC_Get_HeatPadCurr();

/*******************************************/
//	HEATER
/*******************************************/
float f_ADC_Get_HeaterCurr();

/*******************************************/
//	BATTERY
/*******************************************/
float f_ADC_Get_BatVolt();

/*******************************************/
//	REFERENCT VOLT
/*******************************************/
float f_ADC_Get_RefVolt();


#endif

