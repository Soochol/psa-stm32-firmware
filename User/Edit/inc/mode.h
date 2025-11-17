#ifndef __JH_MODE_H
#define __JH_MODE_H

#include "lib_def.h"

//Module Enabled
#define MODE_FSR_USED		1	//FSR
#define HEAT_PAD_USED		0	//Heat-pad

#define TEST_MODE_ONLY		0	//Booting - only test

//B/D Inspiration
#define MODE_TEST_SUB_BD	0
#define MODE_MP3_PLAYING	1


//////////////////////////////
//	DEFINE	: LED COLOR 	//
//////////////////////////////
#define MODE_BOOT_LED_R	0xCD
#define MODE_BOOT_LED_G	0xFF
#define MODE_BOOT_LED_B	0x96

#define MODE_WAIT_LED_R	0x33
#define MODE_WAIT_LED_G	0x99
#define MODE_WAIT_LED_B	0xFF

#define MODE_FORCE_UP_LED_R	0xCC
#define MODE_FORCE_UP_LED_G	0xFF
#define MODE_FORCE_UP_LED_B	0x00

#define MODE_FORCE_ON_LED_R	0xCC
#define MODE_FORCE_ON_LED_G	0xFF
#define MODE_FORCE_ON_LED_B	0x00

#define MODE_FORCE_DOWN_LED_R	0x33
#define MODE_FORCE_DOWN_LED_G	0x99
#define MODE_FORCE_DOWN_LED_B	0xFF

#define MODE_SLEEP_LED_R	0xD3
#define MODE_SLEEP_LED_G	0xD3
#define MODE_SLEEP_LED_B	0xD3

#define MODE_ERROR_LED_R	0xFF
#define MODE_ERROR_LED_G	0x4D
#define MODE_ERROR_LED_B	0x4F


#define MODE_IMU_LED_R		0xD0
#define MODE_IMU_LED_G		0xE0
#define MODE_IMU_LED_B		0x40


#define MODE_TEST_LED_R		0xDB
#define MODE_TEST_LED_G		0x4A
#define MODE_TEST_LED_B		0x75

#define MODE_OFF_LED_R		0x75
#define MODE_OFF_LED_G		0xEB
#define MODE_OFF_LED_B		0x95

//LED Blink interval
#define MODE_LED_BLINK_ITV		500

//////////////////////////////////
//	DEFINE	: INIT PARAMETER 	//
//////////////////////////////////

//Operation Temperature (unit : 1 degree)
#define MODE_TEMP_MAX		70.0f
#define MODE_TEMP_SLEEP		38.0f	//35 -> 38
#define MODE_TEMP_WAITING	38.0f
#define MODE_TEMP_FORCE_UP	52.0F	//50 -> 52	//68.0(TEST)

//Operation Timeout (unit - 1s)
#define MODE_TOUT_FORCE_UP		10
#define MODE_TOUT_FORCE_ON		60	//10s -> 60s
#define MODE_TOUT_FORCE_DOWN	30	//10s -> 30s
#define MODE_TOUT_WAITING		300	//

//ForceDown CoolFan start delay	(unit : 100ms)
#define MODE_FORCE_DOWN_COOL_DELAY_INIT	15	//
#define MODE_FORCE_DOWN_COOL_DELAY_MAX	100	//unit : 100ms

//Cool Fan (range : 0 ~ 10)
#define MODE_COOL_FAN_PWM_MIN	900
#define MODE_COOL_FAN_PWM_MAX	1000

#define MODE_COOL_FAN_STEP_INIT	10
#define MODE_COOL_FAN_STEP_MAX	10

//Heat Pad (range : 0 ~ 10)
#define MODE_HEATPAD_PWM_MIN	800
#define MODE_HEATPAD_PWM_MAX	1000

#define MODE_HEATPAD_STEP_INIT	3
#define MODE_HEATPAD_STEP_MAX	10

//Blow Fan (range : 0 ~ 3)
#define MODE_BLOWFAN_PWM_MIN	300
#define MODE_BLOWFAN_PWM_MAX	1000

#define MODE_BLOWFAN_STEP_INIT	0
#define MODE_BLOWFAN_STEP_MAX	3

//Sound (range : 0 ~ 10)
#if MODE_TEST_SUB_BD
#define MODE_SOUND_VOLUME_INIT	1
#else
#define MODE_SOUND_VOLUME_INIT	5
#endif
//GYRO Angle
#define MODE_GYRO_ANGLE_ACT_INIT	10
#define MODE_GYRO_ANGLE_ACT_MAX		30
#define MODE_GYRO_ANGLE_REL_INIT	5
#define MODE_GYRO_ANGLE_REL_MAX		90


//Booting Timeout
#define MODE_BOOTING_TOUT	20000	//20s

//Healing Timeout
#define MODE_HEALING_TOUT	6000


//////////////////////////////////
//		MODE - BASE 			//
//////////////////////////////////
typedef enum e_modeID{
	modeBOOTING=0,
	modeHEALING,
	modeWAITING,
	modeFORCE_UP,
	modeFORCE_ON,
	modeFORCE_DOWN,
	modeSLEEP,
	modeOFF,
	modeERROR,
	modeTEST,
	modeWAKE_UP,
} e_modeID_t;

typedef struct {
	e_modeID_t e_prev;
	e_modeID_t e_curr;
	e_modeID_t e_next;
}x_modeGUIDE_t;

typedef struct {
	union {
		struct {
			uint8_t	b1_upd	:1;
			uint8_t b1_on	:1;
			uint8_t b1_off	:1;
			uint8_t rsvd	:5;
		}bit;
		uint8_t u8;
	}cr;
	x_modeGUIDE_t guide;
}x_modeWORK_t;

#define modeCR_UPD_ON	(0x03)
#define modeCR_UPD_OFF	(0x05)

void v_Mode_SetNext(e_modeID_t e_id);
e_modeID_t e_Mode_Get_CurrID();


typedef enum {
	modeERR_TEMP_IR		=(1<<0),
	modeERR_TEMP_OUT	=(1<<1),
	modeERR_IMU			=(1<<3),
	modeERR_BLOW_FAN	=(1<<4),
	modeERR_COOL_FAN	=(1<<5),
	modeERR_TOF			=(1<<6),
	modeERR_AUDIO		=(1<<7),
	modeERR_FSR			=(1<<8),
	modeERR_SD_MOUNT	=(1<<9),
	modeERR_MP3_FILE	=(1<<10),
	//add : 1.00.25
	modeERR_HEATER_CURR	=(1<<11),
	//add : 1.00.27
	modeERR_ESP_COMM	=(1<<12),
} e_modeERR_t;
void v_Mode_Set_Error(e_modeERR_t e_type);
void v_Mode_Error_LED_Test(e_modeERR_t test_error);  // LED 에러 패턴 테스트 함수


/////////////////////////////////
//	Operation Timeout
/////////////////////////////////
uint32_t u32_Mode_Get_Waiting_Tout();
uint32_t u32_Mode_Get_ForceUp_Tout();
uint32_t u32_Mode_Get_ForceOn_Tout();
uint32_t u32_Mode_Get_ForceDown_Tout();
void v_Mode_Config_Tout(uint32_t u32_waiting, uint32_t u32_forceUp, uint32_t u32_forceOn, uint32_t u32_forceDown);

/////////////////////////////////
//	Heater Temperature
/////////////////////////////////
float f_Mode_Get_Temp_Max();
float f_Mode_Get_Temp_Sleep();
float f_Mode_Get_Temp_Waiting();
float f_Mode_Get_Temp_ForceUp();

void v_Mode_Set_Temp_Max(float f_temp);
void v_Mode_Set_Temp_Sleep(float f_temp);
void v_Mode_Set_Temp_Waiting(float f_temp);
void v_Mode_Set_Temp_ForceUp(float f_temp);

/////////////////////////////////
//	Parameter - ForceDown Cool-Fan Delay
/////////////////////////////////
uint32_t u32_Mode_Get_ForceDownDelay();
void v_Mode_Set_ForceDownDelay(uint32_t u32_delay);


/////////////////////////////////
//	Switch
/////////////////////////////////
void v_Mode_Enable_BlowFanSW();
void v_Mode_Enable_ToggleSW();
void v_Mode_Enable_PwrSW();


/////////////////////////////////
//	AI - Mode
/////////////////////////////////
typedef enum {
	MODE_AI_ESP=0,
	MODE_AI_STM,
} e_MODE_AI_t;

void v_Mode_Set_AI(int i_mode);
int i_Mode_Get_AI();


/////////////////////////////////
//	Tilt - Angle
/////////////////////////////////
int i_Mode_Get_GyroAngle_Act();
int i_Mode_Get_GyroAngle_Rel();
void v_Mode_Set_GyroAngle_Act(int i_angle);
void v_Mode_Set_GyroAngle_Rel(int i_angle);


/////////////////////////////////
//	Audio Volume
/////////////////////////////////
int i_Mode_Get_Speaker_Vol();
void v_Mode_Set_Speaker_Vol(int i_vol);
void v_Mode_Set_Speaker_Mute(int i_mute);

void v_Mode_Set_MP3_Play(int i_on);

/////////////////////////////////
//	COOL FAN
/////////////////////////////////
uint16_t u16_Mode_Get_CoolFan_Max();
uint16_t u16_Mode_Get_CoolFan_Now();
void v_Mode_Set_CoolFan_Max(uint16_t u16_max);
void v_Mode_Set_CoolFan_Now(uint16_t u16_now);
int i_Mode_Is_CoolFan_Act();
void v_Mode_CoolFan_Enable();
void v_Mode_CoolFan_Disable();


/////////////////////////////////
//	HEAT PAD
/////////////////////////////////
uint16_t u16_Mode_Get_HeatPad_Max();
uint16_t u16_Mode_Get_HeatPad_Now();
void v_Mode_Set_HeatPad_Max(uint16_t u16_max);
void v_Mode_Set_HeatPad_Now(uint16_t u16_now);
void v_Mode_HeatPad_Enable();
void v_Mode_HeatPad_Disable();


/////////////////////////////////
//	BLOW FAN
/////////////////////////////////
uint16_t u16_Mode_Get_BlowFan_Max();
uint16_t u16_Mode_Get_BlowFan_Now();
void v_Mode_Set_BlowFan_Max(uint16_t u16_max);
void v_Mode_Set_BlowFan_Now(uint16_t u16_now);
int i_Mode_Is_BlowFan_Act();
void v_Mode_BlowFan_Enable();
void v_Mode_BlowFan_Disable();


/////////////////////////////////
//	HEATER
/////////////////////////////////
#define MODE_HEATER_LMT_CURRENT	10.0f
#define MODE_HEATER_LMT_TOUT	50	//unit : 100ms	//fix : 9.2

#define MODE_HEATER_PID_COEF_P	100.0f
#define MODE_HEATER_PID_COEF_I	5.0f
#define MODE_HEATER_PID_COEF_D	0.1f
#define MODE_HEATER_PID_COEF_MAX	1000
#define MODE_HEATER_PID_COEF_MIN	0
#define MODE_HEATER_PID_COEF_MAX_INT	900	//integral
#define MODE_HEATER_PID_COEF_MIN_INT	-500//integral

/////////////////////////////////
//	BATTERY
/////////////////////////////////
#define MODE_BAT_VOL_ALERT	32.5f	//unit : volt	//alert
//#define MODE_BAT_VOL_LOW
#define MODE_BAT_VOL_LV1	36.4f	//unit : volt	//10 ~ 44 %
#define MODE_BAT_VOL_LV2	39.4f	//unit : volt	//45 ~ 74 %
//#define MODE_BAT_VOL_LV3	39.4f	//unit : volt	//90%
#define MODE_BAT_OFFSET		0.3f	//unit : volt
#define MODE_BAT_TIME_REFRESH	100	//unit : ms

// Battery voltage compensation for fan speed
#define MODE_BAT_COMP_COOLFAN		0.12f	//unit : volt
#define MODE_BAT_COMP_BLOWFAN_LOW	0.05f	//unit : volt
#define MODE_BAT_COMP_BLOWFAN_MED	0.08f	//unit : volt
#define MODE_BAT_COMP_BLOWFAN_HIGH	0.12f	//unit : volt

/////////////////////////////////
//	TEMPERATURE CONTROL
/////////////////////////////////
// Temperature hysteresis offset to prevent oscillation
#define MODE_TEMP_HYSTERESIS_OFFSET	1.5f	//unit : degree Celsius

/////////////////////////////////
//	TIMING INTERVALS
/////////////////////////////////
// Update and check intervals (unit: milliseconds)
#define MODE_HEATER_PID_UPDATE_ITV		100		// PID controller update interval
#define MODE_HEATER_FB_CHECK_ITV		100		// Heater feedback check interval
#define MODE_BAT_CHECK_ITV				100		// Battery voltage check interval
#define MODE_COOLFAN_HANDLER_ITV		100		// Cool fan handler interval
#define MODE_SENSING_SEND_ITV			100		// Sensor data send to ESP interval
#define MODE_SUBBD_PRINT_ITV			250		// Sub-board debug print interval

// State transition delays
#define MODE_HEALING_INITIAL_TOUT		1000	// Healing mode initial timeout
#define MODE_POWEROFF_DELAY				100		// Power-off delay
#define MODE_LOWPWR_ENTRY_DELAY			1000	// Low power mode entry delay
#define MODE_WAKEUP_DELAY				1000	// Wake-up delay
#define MODE_SOUND_TEST_DELAY			1000	// Sound test MP3 start delay
#define MODE_TILT_CENTER_TIMEOUT		2000	// Tilt center calculation timeout
#define MODE_TILT_DISABLE_TIMEOUT		2000	// Tilt disable timeout

/////////////////////////////////
//	PID CONTROLLER
/////////////////////////////////
// PID time constants
#define MODE_HEATER_PID_TIME_T			0.1f	// PID time constant T
#define MODE_HEATER_PID_TAU				0.1f	// PID tau (filter time)

/////////////////////////////////
//	LED CONTROL
/////////////////////////////////
// LED level calculation step size
#define MODE_LED_LEVEL_STEPS			3.0f	// Steps for dividing 0-10 range into 0-3 levels
#define MODE_LED_LEVEL_MIN				1		// Minimum LED level

/////////////////////////////////
//	SPEAKER VOLUME
/////////////////////////////////
#define MODE_SPEAKER_VOL_MIN			0		// Minimum volume (mute)
#define MODE_SPEAKER_VOL_MAX			10		// Maximum volume
#define MODE_CODEC_VOL_MULTIPLIER		10		// Multiplier for codec volume setting

/////////////////////////////////
//	GYRO/TILT
/////////////////////////////////
#define MODE_GYRO_ANGLE_BASELINE		90		// Gyro angle baseline for tilt calculation

/////////////////////////////////
//	MP3 TRACK NUMBERS
/////////////////////////////////
// MP3 track numbers for different events
#define MODE_MP3_BATTERY_ALERT			14		// Battery low alert sound
#define MODE_MP3_FORCE_UP				19		// Force-up mode start sound
#define MODE_MP3_FORCE_DOWN				20		// Force-down mode start sound
#define MODE_MP3_WAITING_TIMEOUT		21		// Waiting mode timeout sound
#define MODE_MP3_FORCE_ON_TIMEOUT		23		// Force-on mode timeout sound

// Error sound MP3 track numbers
#define MODE_MP3_ERROR_TEMP_IR			27		// IR temperature sensor error
#define MODE_MP3_ERROR_TEMP_OUT			28		// Outdoor temperature sensor error
#define MODE_MP3_ERROR_IMU				29		// IMU sensor error
#define MODE_MP3_ERROR_BLOW_FAN			30		// Blow fan error
#define MODE_MP3_ERROR_COOL_FAN			31		// Cool fan error
#define MODE_MP3_ERROR_TOF				32		// Time-of-flight sensor error

/////////////////////////////////
//	CONVERSION MULTIPLIERS
/////////////////////////////////
#define MODE_TOUT_TO_MS_MULT			1000	// Convert timeout seconds to milliseconds
#define MODE_DELAY_TO_MS_MULT			100		// Convert delay units (100ms) to milliseconds


/////////////////////////////////
//	Mode - Handler
/////////////////////////////////
void v_Mode_Init();
void v_Mode_Handler();


/////////////////////////////////
//	MODE - TEST (JIG)
/////////////////////////////////
void v_Mode_DBG_Enter();
void v_Mode_Set_DBG_TempMax(float f_max);
void v_Mode_Set_DBG_FanSpeed(int i_speed);
void v_Mode_Set_DBG_FoceUp(float f_temp);
void v_Mode_Set_DBG_ForceDown(float f_temp);
void v_Mode_Set_DBG_Waiting(float f_temp);
void v_Mode_Set_DBG_Tout(uint32_t u32_time);
void v_Mode_Set_DBG_ForceUp2(float f_temp);
void v_Mode_Set_DBG_Cool(float f_temp);
void v_Mode_Set_DBG_Enable();


#define modeTEST_ACT		0x0100
#define modeTEST_IDLE		0x0000
#define modeTEST_FORCE_UP	0x0001
#define modeTEST_FORCE_DOWN	0x0002
#define modeTEST_WAITING	0x0003

void v_Mode_Set_DBG_Act(int type);


/////////////////////////////////
//	Only Test
/////////////////////////////////
void v_Mode_JIG_LED_BD();
void v_Mode_CoolFan();
void v_Mode_Sound_Test();


void v_Mode_Heater_Fan_Test();

#endif
