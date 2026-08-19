#ifndef __JH_COMM_ESP_H
#define __JH_COMM_ESP_H

#include "uart.h"
#include "quaternion_mahony.h"

/*******************************************/
//	ESP COMMUNICATION CONSTANTS
/*******************************************/
// Buffer sizes
#define ESP_RX_DATA_BUF_SIZE		32		// Receive data buffer size
#define ESP_SENSING_DATA_BUF_SIZE	96		// Sensing data buffer size
#define ESP_TX_FMT_BUF_SIZE			96		// Transmission format buffer size
#define ESP_TX_TO_RX_BUF_SIZE		64		// Transmit to RX buffer size

// Communication timing
#define ESP_COMM_TIMEOUT			1000	// Communication timeout (ms)
#define ESP_CMD_TEST_ITV			5000	// Command test interval (ms)

// Temperature conversion
#define ESP_TEMP_DECIMAL_LIMIT		10		// Temperature decimal digit limit
#define ESP_TEMP_DECIMAL_DIV		10		// Temperature conversion divisor
#define ESP_TEMP_TO_INT_MULT		100		// Temperature to integer multiplier

// Protocol mode codes. Distinct numbering from the internal e_modeID_t — the two
// must always be converted, never assigned across (internal modeWAITING is 2,
// which is FORCE_UP here). See SD logging spec v1.3 section 7.1.
//
// 7-10 were added for the SD log deviceMode field. Only 0-5 and 7 can ever
// appear in a record: the rest are modes where sampling is stopped, so they show
// up in reqLogStatus(0x43) responses only.
typedef enum {
	ESP_EVT_MODE_SLEEP=0,
	ESP_EVT_MODE_WAITING,
	ESP_EVT_MODE_FORCE_UP,
	ESP_EVT_MODE_FORCE_ON,
	ESP_EVT_MODE_FORCE_DOWN,
	ESP_EVT_MODE_TEST,
	ESP_EVT_MODE_ERROR,
	ESP_EVT_MODE_HEALING,
	ESP_EVT_MODE_BOOTING,
	ESP_EVT_MODE_WAKE_UP,
	ESP_EVT_MODE_OFF,
} e_ESP_EVT_MODE_t;

typedef enum {
	ESP_WARN_BATTERY_LOW=0,
} e_ESP_WARN_t;

void v_ESP_Recive(uint8_t u8_rx);

void v_ESP_Handler();


void v_ESP_Send_InitStart();
void v_ESP_Send_InitEnd();
void v_ESP_Send_EvtModeChange(uint8_t u8_mode);
void v_ESP_Send_Sensing(int16_t* pi16_imu_left, int16_t* pi16_imu_right,\
						uint16_t u16_fsr_left, uint16_t u16_fsr_right,\
						float f_tempOut, float f_tempIn, float f_tempIR,\
						uint16_t u16_tof1, uint16_t u16_tof2, float f_bat,\
						uint8_t u8_imu_left_evt, uint8_t u8_imu_right_evt,\
						_x_XYZ_t angle_left, _x_XYZ_t angle_right);

void v_ESP_Tout_Handler();
void v_ESP_Send_Error(uint16_t u16_error);

// Drains a pending SD logging fault into evtLogError(0x84). Called from
// v_ESP_Handler, so it goes out within one main loop pass of the fault.
void v_ESP_LogError_Handler();

// Streams statLogChunk(0x71) while a backfill request is running. Also called
// from v_ESP_Handler.
void v_ESP_Backfill_Handler();
void v_ESP_Send_Warning(uint8_t u8_warn_type);

#endif


