#include "lib_def.h"
#include "stdio.h"
#include "math.h"


/*
steinhart equeation

use : NTC Thermistor sensor calculator

T = kelbin units(C + 273.15)
A, B, C = coefficients
R = resistance (unit : kOhm)

T1 = the lowest temperature
T2 = the mid temperature
T3 = the highest temperature

R1 = resistance of the lowest temperature
R2 = resistance of the mid temperature
R3 = resistance of the highest temperature

lnR = netural logarithm of a resistance in ohms

equation 1
T = 1 / (A + B * ln(R) + C * [ln(R)]^3)

equation 2
1 / T1 = A + B * ln(R1) + C * [ln(R1)]^3
1 / T2 = A + B * ln(R2) + C * [ln(R2)]^3
1 / T3 = A + B * ln(R3) + C * [ln(R3)]^3


solution
L1 = ln(R1), L2 = ln(R2), L3 = ln(R3)
Y1 = 1 / T1, Y2 = 1 / T2, T3 = 1 / T3
r2 = (Y2 - Y1) / (L2 - L1)
r3 = (Y3 - Y1) / (L3 - L1)


A = 1 / T1 - B * ln(R1) - C * [ln(R1)]^3
B = (1 / T2 - A - C * [ln(R2)]^3 ) / ln(R2)
C = (1 / T3 - A - B * ln(R3) ) / [ln(R3)]^3

organize
C = ( (r3 - r2) / (L3 - L2) ) / (L1 + L2 + L3)
B = r2 - C * (L1^2 + L1*L2 + L2^2)
A = Y1 - L1 * (B + C*L1^2)

*/

static const float KELVIN = 273.15;
static float coefficientA, coefficientB, coefficientC;

//user edit
static const uint8_t T1 = 25;
static const uint8_t T2	= 50;
static const uint8_t T3	= 100;

#if 0
//Device : NTCG164BH103JT1
static const float R1 = 10.000;	//25'C
static const float R2 = 3.481;	//50'C
static const float R3 = 0.626;	//100'C
#endif
//Device : NTCG103JF103FT1
static const float R1 = 10.000;	//25'C
static const float R2 = 4.158;	//50'C
static const float R3 = 0.975;	//100'C

static const float RESOLUTION = 4095.0;//adc driver sampling resolution
static const float RESISTANCE = 10.0;//NTC Thermistor resistance (unit : kOhm)


void _v_Steinhart_Cal_Coefficient(){
	float T1K, T2K, T3K;
	float L1, L2, L3;
	float Y1, Y2, Y3;
	float r2, r3;
	
	T1K = T1 + KELVIN;
	T2K = T2 + KELVIN;
	T3K = T3 + KELVIN;
	
	L1 = log(R1);
	L2 = log(R2);
	L3 = log(R3);
	
	Y1 = 1.0 / T1K;
	Y2 = 1.0 / T2K;
	Y3 = 1.0 / T3K;
	
	r2 = (Y2 - Y1) / (L2 - L1);
	r3 = (Y3 - Y1) / (L3 - L1);
	
	coefficientC = ( (r3 - r2) / (L3 - L2) ) / (L1 + L2 + L3);
#if 0
	coefficientB = r2 - coefficientC * ( pow(L1, 2) + L1*L2 + pow(L2, 2) );
	coefficientA = Y1 - L1 * ( coefficientB + coefficientC * pow(L2, 2) );
#endif
	coefficientB = r2 - coefficientC * ( L1*L1 + L1*L2 + L2*L2 );
	coefficientA = Y1 - L1 * ( coefficientB + coefficientC * L2*L2 );
}

/*
Rntc = NTC Thermistor resistance

V[in] = DC input volt
V[out] = (adc / resolution) * V[in]

1) 
	DC V[in]
	   |
	   |
	 |   |
	 |   | NTC thermistor
	 |   |
	   |
	   |-------- V[out]
	   |
     |   |
	 |   | R (resistance)
	 |   |
	   |
	   |
	Ground

V[out] = V[in] * R / (Rntc + R)
Rntc = R * ( (V[in] / V[out]) - 1 )
Rntc = R * ( (resolution / adc) - 1 )
2)
	DC V[in]
	   |
	   |
	 |   |
	 |   | R (resistance)
	 |   |
	   |
	   |-------- V[out]
	   |
     |   |
	 |   | NTC thermistor
	 |   |
	   |
	   |
	Ground

V[out] = V[in] * Rntc / (Rntc + R)
Rntc = R / ( (V[in] / V[out]) -1 )
Rntc = R / ( (resolution / adc) - 1 )

*/
static float _f_Steinhart_Get_Rntc_DcToRntc(uint16_t adc){
	float Rntc = RESISTANCE * ( (RESOLUTION / (float)adc) -1 );
	return Rntc;
}

static float _f_Steinhart_Get_Rntc_DcToR(uint16_t adc){
	float Rntc = RESISTANCE / ( (RESOLUTION / (float)adc) -1 );
	return Rntc;
}






float _f_Steinhart_Get_Temp_DcToRntc(uint16_t adc){
	float T, Rntc;
	
	Rntc = _f_Steinhart_Get_Rntc_DcToRntc(adc);
	//T = coefficientA + coefficientB * log(Rntc) + coefficientC * pow(log(Rntc), 3);	//memory save
	//T = coefficientA + coefficientB * log(Rntc) + coefficientC * log(Rntc) * log(Rntc) * log(Rntc);
	double rntc_log = log(Rntc);
	T =  coefficientA + coefficientB * rntc_log + coefficientC * rntc_log * rntc_log * rntc_log;
	T = 1 / T;
	return T - KELVIN;
}

float _f_Steinhart_Get_Temp_DcToR(uint16_t adc){
	float T, Rntc;
	
	Rntc = _f_Steinhart_Get_Rntc_DcToR(adc);
	//T = coefficientA + coefficientB * log(Rntc) + coefficientC * pow(log(Rntc), 3);	//memory save
	//T = coefficientA + coefficientB * log(Rntc) + coefficientC * log(Rntc) * log(Rntc) * log(Rntc);
	double rntc_log = log(Rntc);
	T = coefficientA + coefficientB * rntc_log + coefficientC * rntc_log * rntc_log * rntc_log;
	T = 1 / T;
	return T - KELVIN;
}




































