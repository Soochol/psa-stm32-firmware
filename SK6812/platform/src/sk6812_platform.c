#include "main.h"
#include "sk6812_platform.h"
#include "tim.h"
#include "string.h"
#include "mode.h"
/*
 * State  	: 10
 * - top 	: 5
 * - bottom	: 5
 * Battery	: 3
 * Heater	: 3
 * Cooler	: 3
 */

/************************************/
//		LED PWM pusle				//
//	0 : high(0.3us) + low(0.9us)	//
//	1 : high(0.6us) + low(0.6us)	//
//	pwm total tick : 300(1.25us)	//
//	0.3125us 	: 60 tick			//
//	0.625us		: 120 tick			//
/************************************/
#define RGB_PWM_BIT_0	(60)
#define RGB_PWM_BIT_1	(120)
uint16_t u16_pwmBit[3] = {RGB_PWM_BIT_0, RGB_PWM_BIT_1, 0};
/********************************/
//			COMMON				//
//	LED Order					//
//	- top						//
//	- bottom					//
//	- heater					//
//	- cooler					//
//	- battery					//
/********************************/


uint16_t u16_rgbArr[RGB_TOTAL_CNT][3];
uint16_t u16_pwmArrOn[RGB_TOTAL_CNT * 24 + 2];
uint16_t u16_pwmArrOff[RGB_TOTAL_CNT * 24 + 2];

bool b_rgbAct;


void v_RGB_PWM_Out();

uint32_t rgb_callback;
uint8_t R, G, B;
uint16_t dim;
uint16_t dimLut;


const uint8_t u8_led_gamma_lut[100] = {
	0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   2,   2,   2,
	3,   3,   3,   4,   4,   4,   5,   5,   6,   6,   7,   7,   8,   9,   9,  10,
	10,  11,  12,  12,  13,  14,  15,  16,  16,  17,  18,  19,  20,  21,  22,  23,
	24,  24,  26,  27,  28,  29,  30,  31,  32,  33,  34,  36,  37,  38,  39,  40,
	42,  43,  44,  46,  47,  49,  50,  51,  53,  54,  56,  57,  59,  60,  62,  64,
	65,  67,  69,  70,  72,  74,  75,  77,  79,  81,  83,  84,  86,  88,  90,  92,
	94,  96,  98, 100,
};

// Gamma brightness lookup table <https://victornpb.github.io/gamma-table-generator>
// gamma = 2.20 steps = 256 range = 0-255
const uint8_t gamma_lut[256] = {
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
     1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
     3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
     6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
    12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
    20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
    30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
    42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
    56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
    73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
    91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
   113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
   137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
   163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
   192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
   223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255,
  };

void v_RGB_Init(){
	//memset(u16_led_pwm, 0, sizeof(u16_led_pwm));
	v_RGB_Clear();
	v_RGB_Enable_Duty();
	for(int i=0; i<RGB_TOTAL_CNT * 24; i++){
		u16_pwmArrOn[i] = u16_pwmBit[0];
		u16_pwmArrOff[i] = u16_pwmBit[0];
	}
	u16_pwmArrOn[RGB_TOTAL_CNT * 24] = 0;
	u16_pwmArrOff[RGB_TOTAL_CNT * 24] = 0;
}


#define RGB_DUTY_VAL	10
int i_rgb_duty_en;

void v_RGB_Enable_Duty(){
	i_rgb_duty_en = 1;
}

void v_RGB_Disable_Duty(){
	i_rgb_duty_en = 0;
}
/*
 *
 * note
 * - 100ms tick
 */
void v_RGB_Done_Handler(){
	static int act, on;
	static uint32_t tick;
	if(tick < 99){
		++tick;
	}
	else{
		tick = 0;
		act = 1;
		on = 1;
	}
	if(on && tick >= RGB_DUTY_VAL){
		act = 1;
		on = 0;
	}

	if(i_rgb_duty_en){
		if(act){
			act = 0;
			if(on){
				v_Tim4_Ch2_Out(u16_pwmArrOn, RGB_TOTAL_CNT * 24 + 1);
			}
			else{
				v_Tim4_Ch2_Out(u16_pwmArrOff, RGB_TOTAL_CNT * 24 + 1);
			}
		}
	}
	else{
		v_Tim4_Ch2_Out(u16_pwmArrOn, RGB_TOTAL_CNT * 24 + 1);
	}
}


/*
 * brief	: digital rgb pwm output
 * date
 * - create	: 25.04.29
 * - modify	: -
 * note
 * - rgb pwm output
 */
void v_RGB_PWM_Out(){
	if(b_rgbAct){
		b_rgbAct = false;
		for(uint16_t i=0; i<RGB_TOTAL_CNT; i++){
			if(u16_rgbArr[i][0] & 0x0100){
				u16_rgbArr[i][0] &= ~0x0100;
				for(uint16_t h=0, j=0; h<3; h++){
					for(int16_t k=7; k>-1; j++, k--){
						//bit reverse..
						u16_pwmArrOn[i*24 + j] = u16_pwmBit[(u16_rgbArr[i][h] >> k) & 0x01];
						//0 * 	/ 0
						//16	/ 24
						//24
					}
				}
			}
		}
		//v_Tim4_Ch2_Out(u16_pwmArrOn, RGB_TOTAL_CNT * 24);
	}
}

/*
 * brief	: rgb 3-color set
 * date
 * - create	: 25.05.21
 * - modify	: -
 * param
 * - u8_idx	: led total counts
 * - u8_R	: red color
 * - u8_G	: green color
 * - u8_B	: blue color
 */
bool b_RGB_Set_Color(uint8_t u8_idx, uint8_t u8_R, uint8_t u8_G, uint8_t u8_B){
	if(u8_idx >= RGB_TOTAL_CNT){return false;}
	/*
	u16_rgbArr[u8_idx][0] = u8_G | 0x0100;
	u16_rgbArr[u8_idx][1] = u8_R;
	u16_rgbArr[u8_idx][2] = u8_B;
	*/
	u16_rgbArr[u8_idx][0] = gamma_lut[u8_G] | 0x0100;
	u16_rgbArr[u8_idx][1] = gamma_lut[u8_R];
	u16_rgbArr[u8_idx][2] = gamma_lut[u8_B];
	return true;
}


void v_RGB_Set_Top(uint8_t u8_R, uint8_t u8_G, uint8_t u8_B){
	uint8_t R = u8_R, G = u8_G, B = u8_B;
#if MODE_TEST_SUB_BD
	b_RGB_Set_Color(RGB_TOP_1, R, G, B);
	b_RGB_Set_Color(RGB_TOP_2, R, G, B);
	b_RGB_Set_Color(RGB_TOP_3, R, G, B);
	b_RGB_Set_Color(RGB_TOP_4, R, G, B);
	b_RGB_Set_Color(RGB_TOP_5, R, G, B);
#else
	b_RGB_Set_Color(RGB_TOP_1, R, G, B);
	b_RGB_Set_Color(RGB_TOP_2, R, G, B);
	b_RGB_Set_Color(RGB_TOP_3, R, G, B);
	b_RGB_Set_Color(RGB_TOP_4, R, G, B);
	b_RGB_Set_Color(RGB_TOP_5, R, G, B);
#endif
	b_rgbAct = true;
}

void v_RGB_Set_Bot(uint8_t u8_R, uint8_t u8_G, uint8_t u8_B){
	uint8_t R = u8_R, G = u8_G, B = u8_B;
#if MODE_TEST_SUB_BD
	b_RGB_Set_Color(RGB_BOT_1, R, G, B);
	b_RGB_Set_Color(RGB_BOT_2, R, G, B);
	b_RGB_Set_Color(RGB_BOT_3, R, G, B);
	b_RGB_Set_Color(RGB_BOT_4, R, G, B);
	b_RGB_Set_Color(RGB_BOT_5, R, G, B);
#else
	b_RGB_Set_Color(RGB_BOT_1, R, G, B);
	b_RGB_Set_Color(RGB_BOT_2, R, G, B);
	b_RGB_Set_Color(RGB_BOT_3, R, G, B);
	b_RGB_Set_Color(RGB_BOT_4, R, G, B);
	b_RGB_Set_Color(RGB_BOT_5, R, G, B);
#endif
	b_rgbAct = true;
}

void v_RGB_Set_Cool(uint16_t u16_lv){
	uint8_t R = 0x87, G = 0xCE, B = 0xEB;
	if(u16_lv < 1){
		b_RGB_Set_Color(RGB_COOL_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_COOL_2, 0, 0, 0);
		b_RGB_Set_Color(RGB_COOL_3, 0, 0, 0);
	}
	else if(u16_lv < 2){
		b_RGB_Set_Color(RGB_COOL_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_COOL_2, 0, 0, 0);
		b_RGB_Set_Color(RGB_COOL_3, R, G, B);
	}
	else if(u16_lv < 3){
		b_RGB_Set_Color(RGB_COOL_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_COOL_2, R, G, B);
		b_RGB_Set_Color(RGB_COOL_3, R, G, B);
	}
	else{
		b_RGB_Set_Color(RGB_COOL_1, R, G, B);
		b_RGB_Set_Color(RGB_COOL_2, R, G, B);
		b_RGB_Set_Color(RGB_COOL_3, R, G, B);
	}
	b_rgbAct = true;
}

void v_RGB_Set_Heat(uint16_t u16_lv){
	uint8_t R = 0x87, G = 0xCE, B = 0xEB;
	if(u16_lv < 1){
		b_RGB_Set_Color(RGB_HEAT_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_HEAT_2, 0, 0, 0);
		b_RGB_Set_Color(RGB_HEAT_3, 0, 0, 0);
	}
	else if(u16_lv < 2){
		b_RGB_Set_Color(RGB_HEAT_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_HEAT_2, 0, 0, 0);
		b_RGB_Set_Color(RGB_HEAT_3, R, G, B);
	}
	else if(u16_lv < 3){
		b_RGB_Set_Color(RGB_HEAT_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_HEAT_2, R, G, B);
		b_RGB_Set_Color(RGB_HEAT_3, R, G, B);
	}
	else{
		b_RGB_Set_Color(RGB_HEAT_1, R, G, B);
		b_RGB_Set_Color(RGB_HEAT_2, R, G, B);
		b_RGB_Set_Color(RGB_HEAT_3, R, G, B);
	}
	b_rgbAct = true;
}

void v_RGB_Set_Bat(uint16_t u16_lv){
	uint8_t R=0xCC, G=0xFF, B=0x00;
	if(u16_lv < 1){
		b_RGB_Set_Color(RGB_BAT_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_BAT_2, 0, 0, 0);
		b_RGB_Set_Color(RGB_BAT_3, 0, 0, 0);
	}
	else if(u16_lv < 2){
		b_RGB_Set_Color(RGB_BAT_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_BAT_2, 0, 0, 0);
		b_RGB_Set_Color(RGB_BAT_3, R, G, B);
	}
	else if(u16_lv < 3){
		b_RGB_Set_Color(RGB_BAT_1, 0, 0, 0);
		b_RGB_Set_Color(RGB_BAT_2, R, G, B);
		b_RGB_Set_Color(RGB_BAT_3, R, G, B);
	}
	else{
		b_RGB_Set_Color(RGB_BAT_1, R, G, B);
		b_RGB_Set_Color(RGB_BAT_2, R, G, B);
		b_RGB_Set_Color(RGB_BAT_3, R, G, B);
	}
	b_rgbAct = true;
}

void v_RGB_Refresh_Enable(){
	b_rgbAct = true;
}

void v_RGB_Clear(){
	for(int i=0; i<RGB_TOTAL_CNT; i++){
		b_RGB_Set_Color(i, 0, 0, 0);
	}
	b_rgbAct = true;
}

void v_RGB_PWM_Test(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 10)){
		timRef = u32_Tim_1msGet();

		static uint16_t order;
		static uint16_t delay;
		//static uint8_t R, G, B;
		switch(order){
		case 0:
			//Red increase
			if(R < 255){
				R++;
			}
			else{
				order++;
				delay = 0;
			}
			break;
		case 2:
			//Blue decrease
			if(B > 0){
				B--;
			}
			else{
				order++;
				delay = 0;
			}
			break;
		case 4:
			//Green increase
			if(G < 255){
				G++;
			}
			else{
				order++;
				delay = 0;
			}
			break;
		case 6:
			//Red decrease
			if(R > 0){
				R--;
			}
			else{
				order++;
				delay = 0;
			}
			break;
		case 8:
			//Blue increase
			if(B < 255){
				B++;
			}
			else{
				order++;
				delay = 0;
			}
			break;
		case 10:
			//Green decrease
			if(G > 0){
				G--;
			}
			else{
				order++;
				delay = 0;
			}
			break;
		default:
			//if odd
			if(delay < 100){
				delay++;
			}
			else{
				if(order < 11){
					order++;
				}
				else{
					order = 0;
				}
			}
			break;
		}


		static uint16_t num;
		b_RGB_Set_Color(num++, R, G, B);
		if(num > (RGB_TOTAL_CNT - 1)){num=0;}
		b_RGB_Set_Color(num++, R, G, B);
		if(num > (RGB_TOTAL_CNT - 1)){num=0;}
		b_RGB_Set_Color(num++, R, G, B);
		if(num > (RGB_TOTAL_CNT - 1)){num=0;}
		b_RGB_Set_Color(num++, R, G, B);
		if(num > (RGB_TOTAL_CNT - 1)){num=0;}
		b_RGB_Set_Color(num++, R, G, B);
		if(num > (RGB_TOTAL_CNT - 1)){num=0;}

		/*
		b_RGB_Set_Color(0, R, G, B);
		b_RGB_Set_Color(1, G, B, R);
		b_RGB_Set_Color(2, B, R, G);
		b_RGB_Set_Color(3, R, G, B);
		b_RGB_Set_Color(4, G, B, R);
		b_RGB_Set_Color(5, B, R, G);
		b_RGB_Set_Color(6, R, G, B);
		 */
		b_rgbAct = true;
	}
	//v_RGB_PWM_Out();
}



void v_RGB_Test(){
	static uint32_t timRef;
	static uint16_t toggle;
	static uint16_t ledNum;
	static uint8_t R, G, B;

	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 1000)){
		timRef = u32_Tim_1msGet();
		if(ledNum > 3){
			ledNum = 0;
			if(toggle < 4){
				toggle++;
			}
			else{
				toggle = 0;
			}
		}

		if(ledNum == 0){
			R = 150;
			G = 0;
			B = 0;
		}
		else if(ledNum == 1){
			R = 0;
			G = 150;
			B = 0;
		}
		else if(ledNum == 2){
			R = 0;
			G = 0;
			B = 150;
		}

		else if(ledNum == 3){
			R = 0;
			G = 0;
			B = 0;
		}
		ledNum++;

		if(toggle == 0){
			b_RGB_Set_Color(RGB_TOP_1, R, G, B);
			b_RGB_Set_Color(RGB_TOP_2, R, G, B);
			b_RGB_Set_Color(RGB_TOP_3, R, G, B);	//issue - blink
			b_RGB_Set_Color(RGB_TOP_4, R, G, B);	//issue - sound
			b_RGB_Set_Color(RGB_TOP_5, R, G, B);
		}
		else if(toggle == 1){
			b_RGB_Set_Color(RGB_BOT_1, R, G, B);
			b_RGB_Set_Color(RGB_BOT_2, R, G, B);
			b_RGB_Set_Color(RGB_BOT_3, R, G, B);
			b_RGB_Set_Color(RGB_BOT_4, R, G, B);
			b_RGB_Set_Color(RGB_BOT_5, R, G, B);
		}
		else if(toggle == 2){
			b_RGB_Set_Color(RGB_COOL_1, R, G, B);
			b_RGB_Set_Color(RGB_COOL_2, R, G, B);
			b_RGB_Set_Color(RGB_COOL_3, R, G, B);
		}
		else if(toggle == 3){
			b_RGB_Set_Color(RGB_HEAT_1, R, G, B);
			b_RGB_Set_Color(RGB_HEAT_2, R, G, B);
			b_RGB_Set_Color(RGB_HEAT_3, R, G, B);
		}
		else{
			b_RGB_Set_Color(RGB_BAT_1, R, G, B);
			b_RGB_Set_Color(RGB_BAT_2, R, G, B);
			b_RGB_Set_Color(RGB_BAT_3, R, G, B);
		}

		//v_RGB_Refresh_Enable();

	}
}

















