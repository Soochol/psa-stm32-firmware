#ifndef __JH_COMM_ESP_H
#define __JH_COMM_ESP_H

#include "uart.h"


typedef enum {
	ESP_EVT_MODE_SLEEP=0,
	ESP_EVT_MODE_WAITING,
	ESP_EVT_MODE_FORCE_UP,
	ESP_EVT_MODE_FORCE_ON,
	ESP_EVT_MODE_FORCE_DOWN,
	ESP_EVT_MODE_TEST,
	ESP_EVT_MODE_ERROR,
} e_ESP_EVT_MODE_t;

void v_ESP_Recive(uint8_t u8_rx);

void v_ESP_Handler();


void v_ESP_Send_InitStart();
void v_ESP_Send_InitEnd();
void v_ESP_Send_EvtModeChange(uint8_t u8_mode);
void v_ESP_Send_Sensing(int16_t* pi16_imu_left, int16_t* pi16_imu_right,\
						uint16_t u16_fsr_left, uint16_t u16_fsr_right,\
						float f_tempOut, float f_tempIn, float f_tempIR,\
						uint16_t u16_tof1, uint16_t u16_tof2, float f_bat,\
						uint8_t u8_imu_left_evt, uint8_t u8_imu_right_evt);

void v_ESP_Tout_Handler();
void v_ESP_Send_Error(uint16_t u16_error);

void v_ESP_CmdTest();

#endif


