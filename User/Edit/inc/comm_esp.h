#ifndef __JH_COMM_ESP_H
#define __JH_COMM_ESP_H

#include "uart.h"

/*******************************************/
//	ESP COMMUNICATION CONSTANTS
/*******************************************/
// Buffer sizes
#define ESP_RX_DATA_BUF_SIZE		32		// Receive data buffer size
#define ESP_RESP_DATA_BUF_SIZE		32		// Response data buffer size
#define ESP_SENSING_DATA_BUF_SIZE	64		// Sensing data buffer size
#define ESP_TX_FMT_BUF_SIZE			64		// Transmission format buffer size
#define ESP_TX_TO_RX_BUF_SIZE		64		// Transmit to RX buffer size
#define ESP_DATA_LEN_MAX			32		// Maximum data length for validation

// Communication timing
#define ESP_COMM_TIMEOUT			1000	// Communication timeout (ms)
#define ESP_CMD_TEST_ITV			5000	// Command test interval (ms)

// Temperature conversion
#define ESP_TEMP_DECIMAL_LIMIT		10		// Temperature decimal digit limit
#define ESP_TEMP_DECIMAL_DIV		10		// Temperature conversion divisor
#define ESP_TEMP_TO_INT_MULT		100		// Temperature to integer multiplier

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


