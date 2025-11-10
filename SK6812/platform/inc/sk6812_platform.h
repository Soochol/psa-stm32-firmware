#ifndef __JH_SK612_PLATFORM_H
#define __JH_SK612_PLATFORM_H

#include "sk6812_api.h"

#define RGB_TOP_CNT		(5)
#define RGB_BOT_CNT		(5)
#define RGB_BATT_CNT	(3)
#define RGB_HEAT_CNT	(3)
#define RGB_COOL_CNT	(3)
#define RGB_TOTAL_CNT	(RGB_TOP_CNT + RGB_BOT_CNT + RGB_BATT_CNT + RGB_HEAT_CNT + RGB_COOL_CNT)


typedef enum {
	RGB_TOP_1=0,
	RGB_TOP_2,
	RGB_TOP_3,
	RGB_TOP_4,
	RGB_TOP_5,
	RGB_BOT_1,
	RGB_BOT_2,
	RGB_BOT_3,
	RGB_BOT_4,
	RGB_BOT_5,
	RGB_HEAT_1,
	RGB_HEAT_2,
	RGB_HEAT_3,
	RGB_COOL_1,
	RGB_COOL_2,
	RGB_COOL_3,
	RGB_BAT_1,
	RGB_BAT_2,
	RGB_BAT_3,
} e_RGB_IDX_t;


void v_RGB_Init();
void v_RGB_Enable_Duty();
void v_RGB_Disable_Duty();

bool b_RGB_Set_Color(uint8_t u8_idx, uint8_t u8_R, uint8_t u8_G, uint8_t u8_B);

void v_RGB_PWM_Test();
void v_RGB_Done_Handler();
void v_RGB_Dimming();



void v_RGB_Set_Top(uint8_t u8_R, uint8_t u8_G, uint8_t u8_B);
void v_RGB_Set_Bot(uint8_t u8_R, uint8_t u8_G, uint8_t u8_B);
void v_RGB_Set_Cool(uint16_t u16_lv);
void v_RGB_Set_Heat(uint16_t u16_lv);
void v_RGB_Set_Bat(uint16_t u16_lv);

void v_RGB_Refresh_Enable();
void v_RGB_Clear();

void v_RGB_PWM_Out();

void v_RGB_Test();

#endif

