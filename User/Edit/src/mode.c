#include "main.h"
#include "mode.h"
#include "tim.h"
#include "adc.h"
#include "io.h"
#include "uart.h"
#include "comm_dbg.h"
#include "comm_esp.h"
#include "lib_pid.h"
#include "key.h"
#include "i2c.h"
//sesing
#include "icm42670p_platform.h"	//imu
#include "ads111x_platform.h"	//fsr
#include "as6221_platform.h"	//temp in, out
#include "mlx90640_platform.h"	//ir temp
#include "vl53l0x_platform.h"	//tof
#include "minimp3_platform.h"	//mp3 play
#include "es8388_platform.h"	//audio
#include "sk6812_platform.h"	//led
//tilt
//#include "quaternion_mahony.h"
#include "lib_log.h"

#define MODE_LOG_ENABLED	1

// Forward declaration
e_modeERR_t e_Mode_Get_Error(void);

typedef struct {
	uint32_t u32_tim_ref;
	uint32_t u32_timToutRef;
	uint32_t u32_timLedRef;
	uint32_t u32_timLedItv;
	int i_tempRenew;
	int i_sound;
	bool b_work_ovr;
}x_modePUB_t;

typedef struct {
	uint32_t u32_waiting;
	uint32_t u32_forceUp;
	uint32_t u32_forceOn;
	uint32_t u32_forceDown;
} x_modeTOUT_t;


typedef struct {
	float f_max;
	float f_typ;
} x_modeTEMP_t;

typedef struct {
	float f_sleep;
	float f_waiting;
	float f_forceUp;
	float f_max;
} x_modeTEMP_LMT_t;


typedef struct {
	uint16_t u16_on;
	uint16_t u16_now;
	uint16_t u16_max;
} x_modeSTEP_t;


static void v_Mode_CoolFan_PWM(x_modeSTEP_t x_step);
static void v_Mode_HeatPad_PWM(x_modeSTEP_t x_step);


static int i_Mode_Is_Off();

static void v_Mode_SubBD_Print();

static x_modeWORK_t x_modeWork;
static x_modePUB_t x_modePub;










static e_modeERR_t e_modeError;

/*
 * brief	: Initialize mode state machine with specified mode
 * param	: e_id - Initial mode ID to set (e.g., modeBOOTING)
 * return	: void
 * note		: - Sets current mode and triggers update
 *			  - Should be called once during system initialization
 *			  - Bypasses normal mode transition validation
 */
void v_Mode_SetInit(e_modeID_t e_id){
	x_modeWORK_t* px_work = &x_modeWork;
	px_work->guide.e_curr = e_id;
	px_work->cr.u8 = modeCR_UPD_ON;
}

/*
 * brief	: Request transition to next mode
 * param	: e_id - Target mode ID to transition to
 * return	: void
 * note		: - Sets next mode and marks for transition
 *			  - Actual transition happens in v_Mode_Handler()
 *			  - Mode transition may be delayed or validated
 */
void v_Mode_SetNext(e_modeID_t e_id){
	x_modeWORK_t* px_work = &x_modeWork;

	// Debug logging for modeOFF transitions
	if(e_id == modeOFF){
		LOG_INFO("MODE", "Transition to modeOFF from mode=%d", px_work->guide.e_curr);
	}

	px_work->guide.e_next = e_id;
	px_work->cr.u8 = modeCR_UPD_OFF;
}

/*
 * brief	: Execute mode state transition
 * param	: px_work - Pointer to mode state machine work structure
 * return	: void
 * note		: - Moves prev <- curr <- next in state history
 *			  - Sets default next mode to WAITING
 *			  - Triggers update flag for mode entry handlers
 *			  - Should only be called from v_Mode_Handler()
 */
void v_Mode_MoveNext(x_modeWORK_t* px_work){
	LOG_INFO("MODE", "%d -> %d (error=0x%04X)",
	       px_work->guide.e_curr,
	       px_work->guide.e_next,
	       e_Mode_Get_Error());

	if(px_work->guide.e_next == modeERROR) {
		LOG_ERROR("MODE", "Entering ERROR mode - GPS handler will NOT run!");
	}

	px_work->guide.e_prev = px_work->guide.e_curr;
	px_work->guide.e_curr = px_work->guide.e_next;
	px_work->guide.e_next = modeWAITING;

	px_work->cr.u8 = modeCR_UPD_ON;
}

void v_Mode_CurrEnd(x_modeWORK_t* px_work){
	px_work->cr.u8 = modeCR_UPD_OFF;
}

e_modeID_t e_Mode_Get_CurrID(){
	return x_modeWork.guide.e_curr;
}




e_modeERR_t e_Mode_Get_Error(){
	return e_modeError;
}

void v_Mode_Set_Error(e_modeERR_t e_type){
	e_modeError |= e_type;
	// 에러 발생 시 즉시 에러 코드 출력 (디버깅용)
	LOG_ERROR("MODE", "*** ERROR SET: 0x%04X (total=0x%04X) ***", e_type, e_modeError);
	if(e_type & modeERR_TEMP_IR)     LOG_ERROR("MODE", "  -> modeERR_TEMP_IR (열화상)");
	if(e_type & modeERR_TEMP_OUT)    LOG_ERROR("MODE", "  -> modeERR_TEMP_OUT (실외온도)");
	if(e_type & modeERR_TEMP_IN)     LOG_ERROR("MODE", "  -> modeERR_TEMP_IN (실내온도)");
	if(e_type & modeERR_IMU)         LOG_ERROR("MODE", "  -> modeERR_IMU (자이로)");
	if(e_type & modeERR_BLOW_FAN)    LOG_ERROR("MODE", "  -> modeERR_BLOW_FAN (송풍팬)");
	if(e_type & modeERR_COOL_FAN)    LOG_ERROR("MODE", "  -> modeERR_COOL_FAN (쿨링팬)");
	if(e_type & modeERR_TOF)         LOG_ERROR("MODE", "  -> modeERR_TOF (거리센서)");
	if(e_type & modeERR_AUDIO)       LOG_ERROR("MODE", "  -> modeERR_AUDIO (오디오)");
	if(e_type & modeERR_FSR)         LOG_ERROR("MODE", "  -> modeERR_FSR (압력센서)");
	if(e_type & modeERR_SD_MOUNT)    LOG_ERROR("MODE", "  -> modeERR_SD_MOUNT (SD카드)");
	if(e_type & modeERR_MP3_FILE)    LOG_ERROR("MODE", "  -> modeERR_MP3_FILE (MP3파일)");
	if(e_type & modeERR_HEATER_CURR) LOG_ERROR("MODE", "  -> modeERR_HEATER_CURR (히터과전류)");
	if(e_type & modeERR_ESP_COMM)    LOG_ERROR("MODE", "  -> modeERR_ESP_COMM (WiFi통신)");
}


/////////////////////////////////
//	Operation Timeout
/////////////////////////////////
static x_modeTOUT_t x_modeTout;

uint32_t u32_Mode_Get_Waiting_Tout(){
	return x_modeTout.u32_waiting;
}

uint32_t u32_Mode_Get_ForceUp_Tout(){
	return x_modeTout.u32_forceUp;
}

uint32_t u32_Mode_Get_ForceOn_Tout(){
	return x_modeTout.u32_forceOn;
}

uint32_t u32_Mode_Get_ForceDown_Tout(){
	return x_modeTout.u32_forceDown;
}

void v_Mode_Config_Tout(uint32_t u32_waiting, uint32_t u32_forceUp, uint32_t u32_forceOn, uint32_t u32_forceDown){
	x_modeTout.u32_waiting = u32_waiting;
	x_modeTout.u32_forceUp = u32_forceUp;
	x_modeTout.u32_forceOn = u32_forceOn;
	x_modeTout.u32_forceDown = u32_forceDown;
}


/////////////////////////////////
//	Heater Temperature
/////////////////////////////////
static x_modeTEMP_LMT_t x_modeTempLmt;

float f_Mode_Get_Temp_Max(){
	return x_modeTempLmt.f_max;
}

float f_Mode_Get_Temp_Sleep(){
	return x_modeTempLmt.f_sleep;
}

float f_Mode_Get_Temp_Waiting(){
	return x_modeTempLmt.f_waiting;
}

float f_Mode_Get_Temp_ForceUp(){
	return x_modeTempLmt.f_forceUp;
}


void v_Mode_Set_Temp_Max(float f_temp){
	x_modeTempLmt.f_max = f_temp;
}

void v_Mode_Set_Temp_Sleep(float f_temp){
	if(f_temp > x_modeTempLmt.f_max){f_temp = x_modeTempLmt.f_max;}
	x_modeTempLmt.f_sleep = f_temp;
}

void v_Mode_Set_Temp_Waiting(float f_temp){
	if(f_temp > x_modeTempLmt.f_max){f_temp = x_modeTempLmt.f_max;}
	x_modeTempLmt.f_waiting = f_temp;
}

void v_Mode_Set_Temp_ForceUp(float f_temp){
	if(f_temp > x_modeTempLmt.f_max){f_temp = x_modeTempLmt.f_max;}
	x_modeTempLmt.f_forceUp = f_temp;
}


/////////////////////////////////
//	Parameter - ForceDown Cool-Fan Delay
/////////////////////////////////
static uint32_t u32_ForceDownDelay;

uint32_t u32_Mode_Get_ForceDownDelay(){
	return u32_ForceDownDelay;
}

void v_Mode_Set_ForceDownDelay(uint32_t u32_delay){
	if(u32_delay > MODE_FORCE_DOWN_COOL_DELAY_MAX){u32_delay = MODE_FORCE_DOWN_COOL_DELAY_MAX;}
	u32_ForceDownDelay = u32_delay;
}


/////////////////////////////////
//	Switch
/////////////////////////////////
int i_sw_blowFan, i_sw_toggle, i_sw_pwr;

void v_Mode_Enable_BlowFanSW(){
	i_sw_blowFan = 1;
}

void v_Mode_Enable_ToggleSW(){
	i_sw_toggle = 1;
}

void v_Mode_Enable_PwrSW(){
	i_sw_pwr = 1;
}

static int i_Mode_Get_BlowFanSW(){
	return i_sw_blowFan;
}

static int i_Mode_Get_ToggleSw(){
	return i_sw_toggle;
}

static void v_Mode_Clear_BlowFanSW(){
	i_sw_blowFan = 0;
}

static void v_Mode_Clear_ToggleSW(){
	i_sw_toggle = 0;
}

static void v_Mode_Clear_PwrSW(){
	i_sw_pwr = 0;
}


/////////////////////////////////
//	AI - Tilt
/////////////////////////////////
static e_MODE_AI_t e_mode_ai;

int i_Mode_Get_AI(){
	return e_mode_ai;
}

void v_Mode_Set_AI(int i_mode){
	if(i_mode != MODE_AI_STM && i_mode != MODE_AI_ESP){
		e_mode_ai = MODE_AI_STM;
	}
	else{
		e_mode_ai = i_mode;
	}
}



/////////////////////////////////
//	Tilt - Angle
/////////////////////////////////
static int i_actAngle, i_relAngle;
typedef enum {
	MODE_TILT_IDLE=0,
	MODE_TILT_ACT,
	MODE_TILT_REL,
} e_MODE_TILT_t;
e_MODE_TILT_t e_mode_tilt;

#define GYRO_AI_AXIS_X	1


int i_Mode_Get_GyroAngle_Act(){
	return i_actAngle;
}

int i_Mode_Get_GyroAngle_Rel(){
	return i_relAngle;
}

void v_Mode_Set_GyroAngle_Act(int i_angle){
	if(i_angle > MODE_GYRO_ANGLE_ACT_MAX){i_angle = MODE_GYRO_ANGLE_ACT_MAX;}
	i_actAngle = i_angle;
}

void v_Mode_Set_GyroAngle_Rel(int i_angle){
	if(i_angle > MODE_GYRO_ANGLE_REL_MAX){i_angle = MODE_GYRO_ANGLE_REL_MAX;}
	i_relAngle = i_angle;
}


static void v_Mode_GyroAI_Handler(){
	if(e_mode_ai != MODE_AI_STM){return;}
	if(e_Mode_Get_CurrID() >= modeHEALING){
		_x_XYZ_t L = x_IMU_Get_Angle_Left();
#if GYRO_AI_AXIS_X
		float actL = L.x;
#else
		float actL = -L.y;
#endif

		int actAngle = MODE_GYRO_ANGLE_BASELINE - i_actAngle;					//90 - 10
		int relAngle = MODE_GYRO_ANGLE_BASELINE - i_actAngle + i_relAngle;	//90 - 10 + 5
		e_modeID_t mode = e_Mode_Get_CurrID();

		if(actL <= actAngle){
			//active
			e_mode_tilt = MODE_TILT_ACT;
			if(mode != modeFORCE_UP && mode != modeFORCE_ON){
				v_Mode_SetNext(modeFORCE_UP);
			}
		}
		else if(actL > relAngle){
			//release
			e_mode_tilt = MODE_TILT_REL;
			if(mode == modeFORCE_UP || mode == modeFORCE_ON){
				v_Mode_SetNext(modeFORCE_DOWN);
			}
		}
	}
}


/////////////////////////////////
//	Audio Volume
/////////////////////////////////
static int i_speaker_vol = MODE_SOUND_VOLUME_INIT;
static int i_spk_update;
static int i_spk_mute;

int i_Mode_Get_Speaker_Vol(){
	return i_speaker_vol;
}

void v_Mode_Set_Speaker_Vol(int i_vol){
	if(i_vol > MODE_SPEAKER_VOL_MAX){
		i_vol = MODE_SPEAKER_VOL_MAX;
	}
	else if(i_vol < MODE_SPEAKER_VOL_MIN){
		i_vol = MODE_SPEAKER_VOL_MIN;
	}
	i_speaker_vol = i_vol;

	i_spk_update = 1;
}

void v_Mode_Set_Speaker_Mute(int i_mute){
	if(i_spk_mute != i_mute){
		i_spk_mute = i_mute;
		i_spk_update = 1;
	}
}

/*
 * brief	: Handle speaker volume updates via ES8388 codec
 * param	: void
 * return	: void
 * note		: - Updates codec volume when i_spk_update flag is set
 *			  - Respects mute state (sets volume to 0 when muted)
 *			  - Multiplies volume by MODE_CODEC_VOL_MULTIPLIER (10x)
 *			  - Clears update flag after successful codec write
 *			  - Called periodically from main mode handlers
 */
static void v_Mode_Speaker_Vol_Handler(){
	if(i_spk_update){
		int volume;
		if(i_spk_mute)	{volume = MODE_SPEAKER_VOL_MIN;}
		else			{volume = i_speaker_vol;}
		if(i_Codec_Volume_Ctrl(volume * MODE_CODEC_VOL_MULTIPLIER) == 1){
			i_spk_update = 0;
		}
	}
}

int i_mp3_play;
void v_Mode_Set_MP3_Play(int i_on){
	i_mp3_play = i_on;
}

int i_Mode_Get_MP3_Play(){
	return i_mp3_play;
}

/////////////////////////////////
//	COOL FAN
/////////////////////////////////
static x_modeSTEP_t x_modeCoolfan;

uint16_t u16_Mode_Get_CoolFan_Max(){
	return x_modeCoolfan.u16_max;
}

uint16_t u16_Mode_Get_CoolFan_Now(){
	return x_modeCoolfan.u16_now;
}

void v_Mode_Set_CoolFan_Max(uint16_t u16_max){
	x_modeCoolfan.u16_max= u16_max;
}

void v_Mode_Set_CoolFan_Now(uint16_t u16_now){
	// MEDIUM: Validate PWM value to prevent undefined behavior
	if(u16_now > TIM2_ARR_MAX){
		u16_now = TIM2_ARR_MAX;
	}
	x_modeCoolfan.u16_now = u16_now;
}


int i_Mode_Is_CoolFan_Act(){
	if(x_modeCoolfan.u16_on && x_modeCoolfan.u16_now){
		return 1;
	}
	else{
		return 0;
	}
}

void v_Mode_CoolFan_Enable(){
	x_modeCoolfan.u16_on = 1;
}

void v_Mode_CoolFan_Disable(){
	x_modeCoolfan.u16_on = 0;
}

static void v_Mode_CoolFan_PWM(x_modeSTEP_t x_step){
	static uint16_t now_prev;
	static uint16_t max_prev;
	static uint16_t pwm_prev;

	uint16_t now = x_step.u16_now;
	uint16_t max = x_step.u16_max;
	uint16_t pwm;
	static uint16_t led_lv;

	if(x_step.u16_on){
		if(now_prev != now || max_prev != max){
			now_prev = now;
			max_prev = max;
			if(now > 0){
				if(max > 1 && now <= max){
					float unit = (MODE_COOL_FAN_PWM_MAX - MODE_COOL_FAN_PWM_MIN) / (float)(max - 1);
					if(now <= 1){
						pwm_prev = MODE_COOL_FAN_PWM_MIN;
					}
					else{
						pwm_prev = MODE_COOL_FAN_PWM_MIN + (unit * (now - 1));
					}
				}
				else{
					pwm_prev = MODE_COOL_FAN_PWM_MAX;
				}
			}
			else{
				pwm_prev = 0;
			}
		}
		pwm = pwm_prev;
		if(pwm_prev != 0){
			float lv = round(((float)now / max) * MODE_LED_LEVEL_STEPS);
			if(lv < MODE_LED_LEVEL_MIN){
				led_lv = MODE_LED_LEVEL_MIN;
			}
			else{
				led_lv = (uint16_t)lv;
			}
		}
		else{
			led_lv = 0;
		}
		v_RGB_Set_Cool(led_lv);
	}
	else{
		v_RGB_Set_Cool(0);
		pwm = 0;
	}
	v_TIM2_Ch4_Out(pwm);
}

static void v_Mode_CoolFan_Handler(){
	v_Mode_CoolFan_PWM(x_modeCoolfan);
}


/////////////////////////////////
//	HEAT PAD
/////////////////////////////////
static x_modeSTEP_t x_modeHeatPad;

uint16_t u16_Mode_Get_HeatPad_Max(){
	return x_modeHeatPad.u16_max;
}

uint16_t u16_Mode_Get_HeatPad_Now(){
	return x_modeHeatPad.u16_now;
}

void v_Mode_Set_HeatPad_Max(uint16_t u16_max){
	// LOW: Validate PWM value to prevent undefined behavior
	if(u16_max > TIM2_ARR_MAX){
		u16_max = TIM2_ARR_MAX;
	}
	x_modeHeatPad.u16_max= u16_max;
}

void v_Mode_Set_HeatPad_Now(uint16_t u16_now){
	// LOW: Validate PWM value to prevent undefined behavior
	if(u16_now > TIM2_ARR_MAX){
		u16_now = TIM2_ARR_MAX;
	}
	x_modeHeatPad.u16_now = u16_now;
}

void v_Mode_HeatPad_Enable(){
	x_modeHeatPad.u16_on = 1;
}

void v_Mode_HeatPad_Disable(){
	x_modeHeatPad.u16_on = 0;
}

static void v_Mode_HeatPad_PWM(x_modeSTEP_t x_step){
	static uint16_t now_prev;
	static uint16_t max_prev;
	static uint16_t pwm_prev;

	uint16_t now = x_step.u16_now;
	uint16_t max = x_step.u16_max;
	uint16_t pwm;
	static uint16_t led_lv;

	if(x_step.u16_on){
		v_IO_Enable_HeatPad();
		if(now_prev != now || max_prev != max){
			now_prev = now;
			max_prev = max;
			if(now > 0){
				if(max > 1 && now <= max){
					float unit = (MODE_HEATPAD_PWM_MAX - MODE_HEATPAD_PWM_MIN) / (float)(max - 1);
					if(now <= 1){
						pwm_prev = MODE_HEATPAD_PWM_MIN;
					}
					else{
						pwm_prev = MODE_HEATPAD_PWM_MIN + (unit * (now - 1));
					}
				}
				else{
					pwm_prev = MODE_HEATPAD_PWM_MAX;
				}
			}
			else{
				pwm_prev = 0;
			}
		}
		pwm = pwm_prev;
		if(pwm_prev != 0){
			float lv = round(((float)now / max) * MODE_LED_LEVEL_STEPS);
			if(lv < MODE_LED_LEVEL_MIN){
				led_lv = MODE_LED_LEVEL_MIN;
			}
			else{
				led_lv = (uint16_t)lv;
			}
		}
		else{
			led_lv = 0;
		}
		v_RGB_Set_Heat(led_lv);
	}
	else{
		v_IO_Disable_HeatPad();  // LOW: Fixed typo (HeadPad -> HeatPad)
		v_RGB_Set_Heat(0);
		pwm = 0;
	}
	v_TIM2_Ch3_Out(pwm);
}

__attribute__((unused)) static void v_Mode_HeatPad_Handler(){
	v_Mode_HeatPad_PWM(x_modeHeatPad);
}


/////////////////////////////////
//	BLOW FAN
/////////////////////////////////
static x_modeSTEP_t x_modeBlowFan;

uint16_t u16_Mode_Get_BlowFan_Max(){
	return x_modeBlowFan.u16_max;
}

uint16_t u16_Mode_Get_BlowFan_Now(){
	return x_modeBlowFan.u16_now;
}

void v_Mode_Set_BlowFan_Max(uint16_t u16_max){
	x_modeBlowFan.u16_max= u16_max;
}

void v_Mode_Set_BlowFan_Now(uint16_t u16_now){
	// MEDIUM: Validate PWM value to prevent undefined behavior
	if(u16_now > TIM2_ARR_MAX){
		u16_now = TIM2_ARR_MAX;
	}
	x_modeBlowFan.u16_now = u16_now;
}

void v_Mode_BlowFan_Enable(){
	x_modeBlowFan.u16_on = 1;
}

int i_Mode_Is_BlowFan_Act(){
	if(x_modeBlowFan.u16_on && x_modeBlowFan.u16_now){
		return 1;
	}
	else{
		return 0;
	}
}

void v_Mode_BlowFan_Disable(){
	x_modeBlowFan.u16_on = 0;
}

static void v_Mode_BlowFan_PWM(x_modeSTEP_t x_step){
	static uint16_t now_prev;
	static uint16_t max_prev;
	static uint16_t pwm_prev;

	uint16_t now = x_step.u16_now;
	uint16_t max = x_step.u16_max;
	uint16_t pwm;
	static uint16_t led_lv;

	if(x_step.u16_on){
		// LOW: Removed unused commented code
		if(now_prev != now || max_prev != max){
			now_prev = now;
			max_prev = max;
			if(now > 0){
				if(max > 1 && now <= max){
					float unit = (MODE_BLOWFAN_PWM_MAX - MODE_BLOWFAN_PWM_MIN) / (float)(max - 1);
					if(now <= 1){
						pwm_prev = MODE_BLOWFAN_PWM_MIN;
					}
					else{
						pwm_prev = MODE_BLOWFAN_PWM_MIN + (unit * (now - 1));
					}
				}
				else{
					pwm_prev = MODE_BLOWFAN_PWM_MAX;
				}
			}
			else{
				pwm_prev = 0;
			}
		}
		pwm = pwm_prev;

		if(pwm_prev != 0){
			float lv = round(((float)now / max) * MODE_LED_LEVEL_STEPS);
			if(lv < MODE_LED_LEVEL_MIN){
				led_lv = MODE_LED_LEVEL_MIN;
			}
			else{
				led_lv = (uint16_t)lv;
			}
		}
		else{
			led_lv = 0;
		}
		v_RGB_Set_Heat(led_lv);
	}
	else{
		// LOW: Removed unused commented code
		v_RGB_Set_Heat(0);
		pwm = 0;
	}
	v_TIM2_Ch2_Out(pwm);
}

static void v_Mode_BlowFan_Handler(){
	v_Mode_BlowFan_PWM(x_modeBlowFan);
}


/////////////////////////////////
//	HEATER - PID
/////////////////////////////////
static PIDController x_currPID;
static int i_heater_on;
uint16_t pidPWM;
static int i_heater_suspend;
static uint32_t u32_heater_toutRef;

static int i_Mode_Is_Heater(){
	return i_heater_on;
}

static void v_Mode_Heater_Off(){
	v_TIM2_Ch1_Out(0);
	PIDController_Init(&x_currPID);
	pidPWM = 0;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_heater_toutRef, 1000)){
		i_heater_on = 0;
	}
}

static void v_Mode_Heater_Enable_Suspend(){
	i_heater_suspend = 1;
}

static void v_Mode_Heater_Disable_Suspend(){
	i_heater_suspend = 0;
}

static int i_Mode_Is_Heater_Suspend(){
	return i_heater_suspend;
}

static void v_Mode_Heater_PID_Init(){
	x_currPID.Kp = MODE_HEATER_PID_COEF_P;	//rise time
	x_currPID.Ki = MODE_HEATER_PID_COEF_I;	//overshoot
	x_currPID.Kd = MODE_HEATER_PID_COEF_D;	//steady-state error	//0.05 -> 0.1

	x_currPID.T = MODE_HEATER_PID_TIME_T;
	x_currPID.tau = MODE_HEATER_PID_TAU;

	x_currPID.limMax = MODE_HEATER_PID_COEF_MAX;	//pwm max
	x_currPID.limMin = MODE_HEATER_PID_COEF_MIN;		//pwm min	//500

	x_currPID.limMaxInt = MODE_HEATER_PID_COEF_MAX_INT;
	x_currPID.limMinInt = MODE_HEATER_PID_COEF_MIN_INT;

	PIDController_Init(&x_currPID);
}


static float f_Mode_Heater_PID_Handler(float f_tempTarget){
	static uint32_t timRef;
	float temp = f_IR_Temp_Get();
	if(i_Mode_Is_Heater_Suspend()){
		v_Mode_Heater_Off();
	}
	else{
		i_heater_on = 1;
		u32_heater_toutRef = u32_Tim_1msGet();
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, MODE_HEATER_PID_UPDATE_ITV)){
			timRef = u32_Tim_1msGet();
			float output;

			output = PIDController_Update(&x_currPID, f_tempTarget, temp);
			pidPWM = output;
			v_TIM2_Ch1_Out((uint16_t)output);
		}
	}
	return temp;
}



/////////////////////////////////
//	HEATER - Feedback
/////////////////////////////////
static int i_Mode_Is_TempHeater_Over(int* pi_renew, float f_temp_lmt){
	static int over;
	float temp = f_IR_Temp_Get();
	float offset;
	if(*pi_renew)	{offset = 0.0;}
	else			{offset = MODE_TEMP_HYSTERESIS_OFFSET;}

	if(temp >= f_temp_lmt){	//52.0 + 1.5
		over = 1;
		*pi_renew = 0;
	}
	else if(temp < f_temp_lmt - offset){
		over = -1;
	}
	else{
		over = 0;
	}
	return over;
}

static int i_Mode_Is_TempHeater_Under(int* pi_renew, float f_temp_lmt){
	static int over;
	float temp = f_IR_Temp_Get();
	float offset;
	if(*pi_renew)	{offset = 0.0;}
	else			{offset = MODE_TEMP_HYSTERESIS_OFFSET;}

	if(temp <= f_temp_lmt){
		over = 1;
		*pi_renew = 0;
	}
	else if(temp > f_temp_lmt + offset){
		over = -1;
	}
	else{
		over = 0;
	}
	return over;
}

uint32_t u32_heaterFB;
static int i_Mode_TempHeater_FB(){
	static uint32_t timRef;
	static uint32_t tick;

	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, MODE_HEATER_FB_CHECK_ITV)){
		timRef = u32_Tim_1msGet();
		if(f_ADC_Get_HeaterCurr() > MODE_HEATER_LMT_CURRENT){
			tick++;
		}
		else{
			if(tick){tick--;}
		}

		u32_heaterFB = tick;
		//100ms x tick
		if(tick > MODE_HEATER_LMT_TOUT){
			//10s over
			v_Mode_Set_Error(modeERR_HEATER_CURR);
			v_Mode_SetNext(modeERROR);
		}
	}
	return 0;
}


/////////////////////////////////
//	BATTERY
/////////////////////////////////
typedef enum {
	modeBAT_LV_ALERT=0,
	modeBAT_LV_1,
	modeBAT_LV_2,
	modeBAT_LV_3,
	modeBAT_LV_INIT,
	modeBAT_LV_LOW,
} e_modeBAT_LV_t;
static int battery_alarm;


static void v_Mode_Bat_Alarm_Enable(){
	battery_alarm = 1;
}

/*
 * brief	: battery measure and led configure
 * date
 * - create	: 25.07.04
 * - modify	: 25.07.07
 * note
 */
static int i_Mode_Battery(int* pi_sound){
	static int playing;
	static e_modeBAT_LV_t bat_prev = modeBAT_LV_INIT;
	static e_modeBAT_LV_t bat_curr;
	static uint32_t timRef, timItv;
	static uint32_t timLedRef;
	static uint32_t ledToggle;
	static int sound_alert = 1;

	if(!i_Mode_Is_Heater()){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
			timRef = u32_Tim_1msGet();
			timItv = MODE_BAT_TIME_REFRESH;	//100 ms

			float bat = f_ADC_Get_BatVolt();
			float batRef[] = {MODE_BAT_VOL_ALERT, MODE_BAT_VOL_LV1, MODE_BAT_VOL_LV2};
			if(bat_prev != modeBAT_LV_INIT){
				if(i_Mode_Is_CoolFan_Act()){
					bat += MODE_BAT_COMP_COOLFAN;
				}
				if(i_Mode_Is_BlowFan_Act()){
					uint16_t lv = u16_Mode_Get_BlowFan_Now();
					if(lv == 1)		{bat += MODE_BAT_COMP_BLOWFAN_LOW;}
					else if(lv == 2){bat += MODE_BAT_COMP_BLOWFAN_MED;}
					else			{bat += MODE_BAT_COMP_BLOWFAN_HIGH;}
				}

				if(bat_prev > modeBAT_LV_ALERT){
					batRef[bat_prev - 1] -= MODE_BAT_OFFSET;
				}
				if(bat_prev < modeBAT_LV_2){
					batRef[bat_prev + 1] += MODE_BAT_OFFSET;
				}
			}

			if(bat <= batRef[modeBAT_LV_ALERT]){
				bat_curr = modeBAT_LV_ALERT;
			}
			else if(bat <= batRef[modeBAT_LV_1]){
				bat_curr = modeBAT_LV_1;
			}
			else if(bat <= batRef[modeBAT_LV_2]){
				bat_curr = modeBAT_LV_2;	// 	2 lv
			}
			else{
				bat_curr = modeBAT_LV_3;
			}
		}

		//sound
		if(i_Mode_Get_MP3_Play()){
			if(bat_curr == modeBAT_LV_ALERT && sound_alert){
				if(i_MP3_Get_Stat() != MP3_BUSY){
					i_MP3_Begin(MODE_MP3_BATTERY_ALERT);
					playing = 1;
					sound_alert = 0;
				}
			}
#if !MODE_MP3_PLAYING
			if(playing){
				int mp3_stat = i_MP3_Playing();
				if(mp3_stat == MP3_DONE || mp3_stat == MP3_ERR){
					playing = 0;
				}
			}
#endif
		}
	}
	//led
	if(bat_curr != bat_prev){
		bat_prev = bat_curr;
		switch(bat_curr){
		case modeBAT_LV_1:
			v_RGB_Set_Bat(1);
			break;
		case modeBAT_LV_2:
			v_RGB_Set_Bat(2);
			break;
		case modeBAT_LV_3:
			v_RGB_Set_Bat(3);
			break;
		default:
			ledToggle = 0;
			timLedRef = u32_Tim_1msGet();
			v_RGB_Set_Bat(1);
			break;
		}
	}

	if(bat_curr == modeBAT_LV_ALERT){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timLedRef, MODE_LED_BLINK_ITV)){
			timLedRef = u32_Tim_1msGet();
			ledToggle++;
			if(ledToggle & 1)	{v_RGB_Set_Bat(0);}
			else				{v_RGB_Set_Bat(1);}
		}
		v_Mode_Heater_Enable_Suspend();
	}
	else{
		v_Mode_Heater_Disable_Suspend();
	}

	return playing;
}

static void v_Mode_Bat_Handler(){
	i_Mode_Battery(&battery_alarm);
}


/////////////////////////////////
//	Sending Sensing Info
/////////////////////////////////
static void v_Mode_Sensing_toESP(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, MODE_SENSING_SEND_ITV)){
		timRef = u32_Tim_1msGet();

		uint8_t imu_evt_left = u8_IMU_Get_EVT_Left();
		uint8_t imu_evt_right = u8_IMU_Get_EVT_Right();
		if(imu_evt_left){
			v_IMU_Clear_EVT_Left();
		}
		if(imu_evt_right){
			v_IMU_Clear_EVT_Right();
		}

		//add imu event left and right
		v_ESP_Send_Sensing(pi16_IMU_Get_Left(), pi16_IMU_Get_Right(), \
						u16_FSR_Get_Left(), u16_FSR_Get_Right(), \
						f_Temp_Out_Get(), f_Temp_In_Get(), f_IR_Temp_Get(), \
						u16_TOF_Get_1(), u16_TOF_Get_2(), f_ADC_Get_BatVolt(),\
						imu_evt_left, imu_evt_right);

		// Fall detection - emergency stop (AFTER sending to ESP)
		if((imu_evt_left & 0x01) || (imu_evt_right & 0x01)){
			LOG_ERROR("MODE", "FALL DETECTED! L=0x%02X R=0x%02X - Emergency stop!", imu_evt_left, imu_evt_right);
			v_Mode_Set_Error(modeERR_FALL);
			v_Mode_SetNext(modeERROR);
		}
	}
}


/////////////////////////////////
//	Sensing Handler
/////////////////////////////////
void v_Mode_Sensing_Handler(){
	if(i_Mode_Is_Off()){return;}
	v_IMU_Handler();
	v_FSR_Data_Handler();
	v_Temp_InOut_Handler();
	v_Temp_IR_Data_Handler();
	v_TOF_Handler();

	//to esp
	v_Mode_Sensing_toESP();


	v_Mode_GyroAI_Handler();
}


/////////////////////////////////
//	Mode - Booting
/////////////////////////////////
typedef enum {
	modeCONFIG_IMU		=0x01,
	modeCONFIG_IR_TEMP	=0x02,
	modeCONFIG_TOF		=0x04,
	modeCONFIG_TEMP_OUT	=0x08,
	modeCONFIG_AUDIO	=0x10,
#if MODE_FSR_USED
	modeCONFIG_FSR		=0x20,
	modeCONFIG_CPLT		=0x3F,
#else
	modeCONFIG_CPLT		=0x1F,
#endif
} e_modeCONFIG_t;

static void v_Mode_Booting_Led(){
	//always on
	v_RGB_Set_Top(MODE_BOOT_LED_R, MODE_BOOT_LED_G, MODE_BOOT_LED_B);
	v_RGB_Set_Bot(MODE_BOOT_LED_R, MODE_BOOT_LED_G, MODE_BOOT_LED_B);
}

static void v_Mode_Booting(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint16_t ready_mask;
	static uint16_t tilt;
	static uint32_t tout;
	static int mp3_wait;
	static uint32_t timTiltRef;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			//init
			ready_mask = 0;
			//time out
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			tout = MODE_BOOTING_TOUT;
			//led
			v_RGB_Clear();
			v_Mode_Booting_Led();
			//tilt
			tilt = 1;
			//esp send
			v_ESP_Send_InitStart();
			//announce wait
			if(i_Mode_Get_MP3_Play()){
				mp3_wait = 1;
			}
			else{
				mp3_wait = 0;
			}
		}
		// LOW: Removed empty else block
	}
	if(px_work->cr.bit.b1_on){
		//initialize
		if(ready_mask != modeCONFIG_CPLT){
			e_COMM_STAT_t ret = COMM_STAT_OK;
			if(!(ready_mask & modeCONFIG_IMU)){
				// Probe IMU devices ONCE before initialization
				static bool b_imu_probed = false;
				static int imu_probe_result = -1;  // -1: not probed, 0: ACK, 1: NACK

				if(!b_imu_probed){
					extern I2C_HandleTypeDef hi2c2;
					int ret_L = i_I2C_ProbeDevice(&hi2c2, 2, ADDR_IMU_LEFT, "IMU_LEFT");
					int ret_R = i_I2C_ProbeDevice(&hi2c2, 2, ADDR_IMU_RIGHT, "IMU_RIGHT");

					// If both IMU sensors NACK, skip initialization to avoid timeout
					if(ret_L != 0 && ret_R != 0){
						imu_probe_result = 1;  // Both NACK
						LOG_WARN("MODE", "Both IMU sensors NACK - skipping init to avoid watchdog");
						LOG_ERROR("MODE", "System will enter ERROR mode with IMU error");
						ready_mask |= modeCONFIG_IMU;  // Skip init
						v_Mode_Set_Error(modeERR_IMU);  // Set error flag
					}
					else{
						imu_probe_result = 0;  // At least one ACK
					}
					b_imu_probed = true;
				}

				// Only attempt initialization if probe succeeded
				if(imu_probe_result == 0){
					ret = e_IMU_Ready();
					if(ret == COMM_STAT_DONE){ready_mask |= modeCONFIG_IMU;}
					else if(ret == COMM_STAT_ERR){
						LOG_ERROR("MODE", "IMU init FAILED on I2C2 - Device B hardware issue?");
						v_Mode_Set_Error(modeERR_IMU);
					}
				}
			}
			else{
#if MODE_FSR_USED
				if(!(ready_mask & modeCONFIG_FSR)){
					// Probe FSR devices ONCE before initialization
					static bool b_fsr_probed = false;
					if(!b_fsr_probed){
						extern I2C_HandleTypeDef hi2c2;
						i_I2C_ProbeDevice(&hi2c2, 2, ADDR_FSR_LEFT, "FSR_LEFT");
						i_I2C_ProbeDevice(&hi2c2, 2, ADDR_FSR_RIGHT, "FSR_RIGHT");
						b_fsr_probed = true;
					}

					ret = e_FSR_Ready();
					if(ret == COMM_STAT_DONE)		{ready_mask |= modeCONFIG_FSR;}
					else if(ret == COMM_STAT_ERR)	{
						LOG_ERROR("MODE", "FSR init FAILED on I2C2 - ADC issue");
						v_Mode_Set_Error(modeERR_FSR);
					}
				}
#endif
			}
			if(!(ready_mask & modeCONFIG_AUDIO)){
				e_COMM_STAT_t ret = e_Codec_Ready();
				if(ret == COMM_STAT_DONE)		{ready_mask |= modeCONFIG_AUDIO;}
				else if(ret == COMM_STAT_ERR)	{v_Mode_Set_Error(modeERR_AUDIO);}
			}
			if(!(ready_mask & modeCONFIG_TEMP_OUT)){
				// Probe Temperature sensors ONCE before initialization
				static bool b_temp_probed = false;
				if(!b_temp_probed){
					extern I2C_HandleTypeDef hi2c1;
					i_I2C_ProbeDevice(&hi2c1, 1, ADDR_TEMP_INDOOR, "TEMP_INDOOR");
					i_I2C_ProbeDevice(&hi2c1, 1, ADDR_TEMP_OUTDOOR, "TEMP_OUTDOOR");
					b_temp_probed = true;
				}

				e_COMM_STAT_t ret = e_Temp_InOut_Ready();
				if(ret == COMM_STAT_DONE)		{ready_mask |= modeCONFIG_TEMP_OUT;}
				else if(ret == COMM_STAT_ERR)	{
					LOG_ERROR("MODE", "Temperature sensor init FAILED on I2C1");
					v_Mode_Set_Error(modeERR_TEMP_OUT);
				}
			}
			else{
				// TOF sensor ENABLED
#if 1
				if(!(ready_mask & modeCONFIG_TOF)){
					extern I2C_HandleTypeDef hi2c1;
					i_I2C_ProbeDevice(&hi2c1, 1, ADDR_TOF1, "TOF");

					e_COMM_STAT_t ret = e_TOF_Ready();
					if(ret == COMM_STAT_DONE)		{ready_mask |= modeCONFIG_TOF;}
					else if(ret == COMM_STAT_ERR)	{
						LOG_ERROR("MODE", "TOF sensor init FAILED on I2C1");
						v_Mode_Set_Error(modeERR_TOF);
					}
				}
#else
				// Skip TOF - assume always ready
				ready_mask |= modeCONFIG_TOF;
#endif
			}
			if(ready_mask == (modeCONFIG_IMU | modeCONFIG_AUDIO | modeCONFIG_TEMP_OUT | modeCONFIG_TOF)){
				if(!(ready_mask & modeCONFIG_IR_TEMP)){
					// Probe MLX90640 IR camera ONCE before initialization
					static bool b_mlx_probed = false;
					if(!b_mlx_probed){
						extern I2C_HandleTypeDef hi2c4;
						i_I2C_ProbeDevice(&hi2c4, 4, ADDR_IR_TEMP, "MLX90640");
						b_mlx_probed = true;
					}

					e_COMM_STAT_t ret = e_IR_Temp_Ready();
					if(ret == COMM_STAT_DONE)		{ready_mask |= modeCONFIG_IR_TEMP;}
					else if(ret == COMM_STAT_ERR)	{
						LOG_ERROR("MODE", "IR Temperature init FAILED on I2C4 - MLX90640 issue");
						v_Mode_Set_Error(modeERR_TEMP_IR);
					}
				}
			}
		}
		else{
			if(tilt){
				tilt = 0;
				v_IMU_Tilt_Center_Enable();
				timTiltRef = u32_Tim_1msGet();

				if(i_Mode_Get_MP3_Play()){
					i_MP3_Begin(1);
				}
			}
#if !MODE_MP3_PLAYING
			if(i_Mode_Get_MP3_Play()){
				int mp3_stat = i_MP3_Playing();
				if(mp3_stat == MP3_DONE || mp3_stat == MP3_ERR){
					mp3_wait = 0;
				}
			}
#endif
			if(i_MP3_Is_Ready()){
				mp3_wait = 0;
			}
		}

		if(tilt == 0 && mp3_wait == 0 && ready_mask == modeCONFIG_CPLT && !i_IMU_Tilt_Is_Center()){
			v_Mode_SetNext(modeHEALING);
		}

		//timeout
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, tout)){
			//timeout to error
			LOG_ERROR("MODE", "Sensor init timeout - ready_mask=0x%04X (expect 0x%04X)",
			       ready_mask, modeCONFIG_CPLT);
			LOG_ERROR("MODE", "Missing sensors:");
			if(!(ready_mask & modeCONFIG_IMU))    LOG_ERROR("MODE", "  - IMU");
			if(!(ready_mask & modeCONFIG_FSR))    LOG_ERROR("MODE", "  - FSR");
			if(!(ready_mask & modeCONFIG_AUDIO))  LOG_ERROR("MODE", "  - Audio Codec");
			if(!(ready_mask & modeCONFIG_TEMP_OUT)) LOG_ERROR("MODE", "  - Temp Sensor");
			if(!(ready_mask & modeCONFIG_TOF))    LOG_ERROR("MODE", "  - TOF Sensor");
			if(!(ready_mask & modeCONFIG_IR_TEMP)) LOG_ERROR("MODE", "  - IR Temp");

			e_modeERR_t err=0;
			if(!(ready_mask & modeCONFIG_IMU)){err |= modeERR_IMU;}
			if(!(ready_mask & modeCONFIG_IR_TEMP)){err |= modeERR_TEMP_IR;}
			if(!(ready_mask & modeCONFIG_TOF)){err |= modeERR_TOF;}
			if(!(ready_mask & modeCONFIG_AUDIO)){err |= modeERR_AUDIO;}
			if(!(ready_mask & modeCONFIG_TEMP_OUT)){err |= modeERR_TEMP_OUT;}
#if MODE_FSR_USED
			if(!(ready_mask & modeCONFIG_FSR)){err |= modeERR_FSR;}
#endif
			v_Mode_Set_Error(err);
		}

		//error check
		if(e_Mode_Get_Error()){
			v_I2C_DiagDump();  // Dump I2C bus status before entering ERROR mode
			v_Mode_SetNext(modeERROR);
		}

		//tilt calculation
		v_IMU_Handler();
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timTiltRef, MODE_TILT_CENTER_TIMEOUT)){
			v_IMU_Tilt_Center_Disable();
		}
	}
	else if(px_work->cr.bit.b1_off){
		if(ready_mask == modeCONFIG_CPLT){
			ready_mask = 0;
			if(i_Mode_Get_MP3_Play()){
				i_MP3_Begin(2);
			}
		}
		if(i_Mode_Get_MP3_Play()){
			int mp3_stat = i_MP3_Get_Stat();
			if(mp3_stat == MP3_DONE || mp3_stat == MP3_ERR || mp3_stat == MP3_IDLE){
				i_MP3_ForceStop();
				v_Mode_MoveNext(px_work);
				v_ESP_Send_InitEnd();
				v_DBG_BootComplete();
			}
		}
		else{
			v_Mode_MoveNext(px_work);
			v_ESP_Send_InitEnd();
			v_DBG_BootComplete();
		}
	}
}


/////////////////////////////////
//	MODE - Healing
/////////////////////////////////
static void v_Mode_Healing(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint32_t tout;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;

		if(px_work->cr.bit.b1_on){
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			tout = MODE_HEALING_INITIAL_TOUT;
			//temperature condition renew
			px_pub->i_tempRenew = 1;
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, tout)){
			v_Mode_SetNext(modeWAITING);
		}
	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
	}

	v_Mode_Sensing_Handler();
	v_Mode_Bat_Handler();
}


/////////////////////////////////
//	MODE - Waiting
/////////////////////////////////
static void v_Mode_Waiting_Led(){
	//always on
	v_RGB_Set_Top(MODE_WAIT_LED_R, MODE_WAIT_LED_G, MODE_WAIT_LED_B);
	v_RGB_Set_Bot(MODE_WAIT_LED_R, MODE_WAIT_LED_G, MODE_WAIT_LED_B);
}


static void v_Mode_Waiting(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint32_t tout;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			//timeout
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			tout = u32_Mode_Get_Waiting_Tout() * MODE_TOUT_TO_MS_MULT;
			if(i_Mode_Get_MP3_Play())	{px_pub->i_sound = 1;}
			else						{px_pub->i_sound = 0;}
			//status led
			v_Mode_Waiting_Led();
			//temperature condition renew
			px_pub->i_tempRenew = 1;
			//mode change to esp
			v_ESP_Send_EvtModeChange(ESP_EVT_MODE_WAITING);
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, tout)){
			//add end condition
			if(px_pub->i_sound){
				// MEDIUM: Stop any currently playing MP3 before timeout sound
				i_MP3_ForceStop();
				if(i_MP3_Is_Ready()){
					i_MP3_Begin(MODE_MP3_WAITING_TIMEOUT);
					px_pub->i_sound = 0;
				}
			}
			else{
				if(i_MP3_Is_Ready()){
					v_Mode_SetNext(modeSLEEP);
				}
			}
		}

		//heater temp check	- OK
		int temp = i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, f_Mode_Get_Temp_Waiting());
		if(temp > 0){
			v_Mode_Heater_Off();
		}
		else if(temp < 0){
			f_Mode_Heater_PID_Handler(f_Mode_Get_Temp_Waiting());
		}
	}
	else if(px_work->cr.bit.b1_off){
		if(i_MP3_Get_Stat() != MP3_BUSY){
			//end condition
			v_Mode_MoveNext(px_work);
			v_Mode_CoolFan_Disable();
		}
	}
	v_Mode_Sensing_Handler();
	v_Mode_BlowFan_Handler();
	v_Mode_CoolFan_Handler();
#if HEAT_PAD_USED
	v_Mode_HeatPad_Handler();
#endif
	v_Mode_Bat_Handler();
}


/////////////////////////////////
//	MODE - ForceUp
/////////////////////////////////
static void v_Mode_ForceUp_Led(uint16_t u16_toggle){
	//blink every 0.5 s
	if(u16_toggle & 1){
		v_RGB_Set_Top(0, 0, 0);
		v_RGB_Set_Bot(0, 0, 0);
	}
	else{
		v_RGB_Set_Top(MODE_FORCE_UP_LED_R, MODE_FORCE_UP_LED_G, MODE_FORCE_UP_LED_B);
		v_RGB_Set_Bot(MODE_FORCE_UP_LED_R, MODE_FORCE_UP_LED_G, MODE_FORCE_UP_LED_B);
	}
}

static void v_Mode_ForceUp(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint32_t tout;
	static uint16_t ledToggle;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			//timeout
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			tout = u32_Mode_Get_ForceUp_Tout() * MODE_TOUT_TO_MS_MULT;
			//sound
			if(i_Mode_Get_MP3_Play() && i_Mode_Get_ToggleSw()){
				px_pub->i_sound = 1;
				v_Mode_Clear_ToggleSW();
			}
			else{
				px_pub->i_sound = 0;
			}
			//led
			px_pub->u32_timLedItv = 0;
			ledToggle = 0;
			//temperature condition renew
			px_pub->i_tempRenew = 1;
			//mode change to esp
			v_ESP_Send_EvtModeChange(ESP_EVT_MODE_FORCE_UP);
			//temperature keep
			v_Mode_CoolFan_Disable();
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, tout)){
			v_Mode_SetNext(modeFORCE_ON);
		}

		int temp = i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, f_Mode_Get_Temp_ForceUp());
		if(temp > 0){
			v_Mode_Heater_Off();
			v_Mode_SetNext(modeFORCE_ON);
		}
		else if(temp < 0){
			f_Mode_Heater_PID_Handler(f_Mode_Get_Temp_ForceUp());
		}

		if(px_pub->i_sound){
			if(i_MP3_Is_Ready()){
				i_MP3_Begin(MODE_MP3_FORCE_UP);
				px_pub->i_sound = 0;
			}
		}
	}
	else if(px_work->cr.bit.b1_off){
		if(px_work->guide.e_next != modeFORCE_ON){
			i_MP3_ForceStop();
		}
		v_Mode_MoveNext(px_work);
		v_Mode_Heater_Off();
	}

	//led
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timLedRef, px_pub->u32_timLedItv)){
		px_pub->u32_timLedRef = u32_Tim_1msGet();
		px_pub->u32_timLedItv = MODE_LED_BLINK_ITV;
		v_Mode_ForceUp_Led(ledToggle++);
		// LOW: Removed unused commented code
	}

	v_Mode_Sensing_Handler();
	v_Mode_BlowFan_Handler();
	v_Mode_CoolFan_Handler();
#if HEAT_PAD_USED
	v_Mode_HeatPad_Handler();
#endif
	v_Mode_Bat_Handler();
}


/////////////////////////////////
//	MODE - ForceOn
/////////////////////////////////
static void v_Mode_ForceOn_Led(){
	//always on
	v_RGB_Set_Top(MODE_FORCE_ON_LED_R, MODE_FORCE_ON_LED_G, MODE_FORCE_ON_LED_B);
	v_RGB_Set_Bot(MODE_FORCE_ON_LED_R, MODE_FORCE_ON_LED_G, MODE_FORCE_ON_LED_B);
}

static void v_Mode_ForceOn(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint32_t tout;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;

		if(px_work->cr.bit.b1_on){
			//timeout
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			tout = u32_Mode_Get_ForceOn_Tout() * MODE_TOUT_TO_MS_MULT;
			if(i_Mode_Get_MP3_Play())	{px_pub->i_sound = 1;}
			else						{px_pub->i_sound = 0;}
			//led
			v_Mode_ForceOn_Led();
			//mode change to esp
			v_ESP_Send_EvtModeChange(ESP_EVT_MODE_FORCE_ON);
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, tout)){
			if(px_pub->i_sound){
				// MEDIUM: Stop any currently playing MP3 before timeout sound
				i_MP3_ForceStop();
				if(i_MP3_Is_Ready()){
					i_MP3_Begin(MODE_MP3_FORCE_ON_TIMEOUT);
					px_pub->i_sound = 0;
				}
			}
			else{
				if(i_MP3_Is_Ready()){
					v_Mode_SetNext(modeSLEEP);
				}
			}
		}
		else{
			int temp = i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, f_Mode_Get_Temp_ForceUp());
			if(temp > 0){
				v_Mode_Heater_Off();
			}
			else if(temp < 0){
				f_Mode_Heater_PID_Handler(f_Mode_Get_Temp_ForceUp());
			}
#if !MODE_MP3_PLAYING
			if(i_Mode_Get_MP3_Play()){
				i_MP3_Playing();
			}
#endif
		}
	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
		v_Mode_Heater_Off();
		i_MP3_ForceStop();
	}

	v_Mode_Sensing_Handler();
	v_Mode_BlowFan_Handler();
	v_Mode_CoolFan_Handler();
#if HEAT_PAD_USED
	v_Mode_HeatPad_Handler();
#endif
	v_Mode_Bat_Handler();
}


/////////////////////////////////
//	MODE - ForceDown
/////////////////////////////////
static void v_Mode_ForceDown_Led(uint16_t u16_toggle){
	//blink every 0.5 s
	if(u16_toggle & 1){
		v_RGB_Set_Top(0, 0, 0);
		v_RGB_Set_Bot(0, 0, 0);
	}
	else{
		v_RGB_Set_Top(MODE_FORCE_DOWN_LED_R, MODE_FORCE_DOWN_LED_G, MODE_FORCE_DOWN_LED_B);
		v_RGB_Set_Bot(MODE_FORCE_DOWN_LED_R, MODE_FORCE_DOWN_LED_G, MODE_FORCE_DOWN_LED_B);
	}
}

static void v_Mode_ForceDown(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint32_t tout;
	static uint16_t ledToggle;
	static uint32_t coolDelayRef, coolDelay;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;

		if(px_work->cr.bit.b1_on){
			//timeout
			px_pub->u32_tim_ref = u32_Tim_1msGet();
			tout = u32_Mode_Get_ForceDown_Tout() * MODE_TOUT_TO_MS_MULT;
			//led
			px_pub->u32_timLedItv = 0;
			ledToggle = 0;
			//temperature condition renew
			px_pub->i_tempRenew = 1;
			//mode change to esp
			v_ESP_Send_EvtModeChange(ESP_EVT_MODE_FORCE_DOWN);
			//delay
			coolDelayRef = u32_Tim_1msGet();
			coolDelay = u32_Mode_Get_ForceDownDelay() * MODE_DELAY_TO_MS_MULT;
			//sound
			if(i_Mode_Get_MP3_Play() && i_Mode_Get_ToggleSw()){
				px_pub->i_sound = 1;
				v_Mode_Clear_ToggleSW();
			}
			else{
				px_pub->i_sound = 0;
			}
		}
		// LOW: Removed empty else block
	}
	if(px_work->cr.bit.b1_on){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_tim_ref, tout)){
			v_Mode_SetNext(modeWAITING);
		}
		else{
#if !MODE_MP3_PLAYING
			//mp3 play
			if(i_Mode_Get_MP3_Play()){
				i_MP3_Playing();
			}
#endif
		}

		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), coolDelayRef, coolDelay)){
			int temp = i_Mode_Is_TempHeater_Under(&px_pub->i_tempRenew, f_Mode_Get_Temp_Waiting());
			if(temp > 0){
				v_Mode_CoolFan_Disable();
				v_Mode_SetNext(modeWAITING);
			}
			else if(temp < 0){
				v_Mode_CoolFan_Enable();
			}
		}

		if(px_pub->i_sound){
			if(i_MP3_Is_Ready()){
				i_MP3_Begin(MODE_MP3_FORCE_DOWN);
				px_pub->i_sound = 0;
			}
		}
	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
		v_Mode_CoolFan_Disable();
		i_MP3_ForceStop();
	}

	//led
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timLedRef, px_pub->u32_timLedItv)){
		px_pub->u32_timLedRef = u32_Tim_1msGet();
		px_pub->u32_timLedItv = MODE_LED_BLINK_ITV;
		v_Mode_ForceDown_Led(ledToggle++);
		// LOW: Removed unused commented code
	}

	v_Mode_Sensing_Handler();
	v_Mode_BlowFan_Handler();
	v_Mode_CoolFan_Handler();
#if HEAT_PAD_USED
	v_Mode_HeatPad_Handler();
#endif
	v_Mode_Bat_Handler();
}


/////////////////////////////////
//	MODE - Sleep
/////////////////////////////////
static void v_Mode_Sleep_Led(){
	//always on
	v_RGB_Set_Top(MODE_SLEEP_LED_R, MODE_SLEEP_LED_G, MODE_SLEEP_LED_B);
	v_RGB_Set_Bot(MODE_SLEEP_LED_R, MODE_SLEEP_LED_G, MODE_SLEEP_LED_B);
}

static void v_Mode_Sleep(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			//led
			v_Mode_Sleep_Led();
			//temperature condition renew
			px_pub->i_tempRenew = 1;
			//mode change to esp
			v_ESP_Send_EvtModeChange(ESP_EVT_MODE_SLEEP);
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		int temp = i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, f_Mode_Get_Temp_Sleep());
		if(temp > 0){
			v_Mode_Heater_Off();
		}
		else if(temp < 0){
			f_Mode_Heater_PID_Handler(f_Mode_Get_Temp_Sleep());
		}

	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
	}

	v_Mode_Sensing_Handler();
	v_Mode_BlowFan_Handler();
	v_Mode_CoolFan_Handler();
#if HEAT_PAD_USED
	v_Mode_HeatPad_Handler();
#endif
	v_Mode_Bat_Handler();
}


/////////////////////////////////
//	MODE - Off
/////////////////////////////////
int i_mode_off;

static int i_Mode_Is_Off(){
	return i_mode_off;
}

void v_Mode_Off(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static int pwr_off, low_pwr;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			v_Mode_Clear_PwrSW();
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			//led clear
			px_pub->u32_timLedRef = u32_Tim_1msGet();
			v_RGB_Disable_Duty();
			v_RGB_Clear();
			//signal
			//pwr_off = 1;
			//sensor stop..
			i_mode_off = 1;
			// LOW: Removed unused commented code
			pwr_off = low_pwr = 0;
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		//HAL_NVIC_SystemReset();
		//sensor off
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timLedRef, MODE_POWEROFF_DELAY)){
			if(pwr_off == 0){
				pwr_off = 1;
				v_IO_Disable_12V();
				v_I2C_Deinit();
				v_AUDIO_Init();					//low
				v_I2C1_Pin_Deinit();
				v_I2C2_Pin_Deinit();
				v_I2C3_Pin_Deinit();
				v_I2C4_Pin_Deinit();
				v_I2C5_Pin_Deinit();
				v_IO_PWR_WakeUp_Enable();

				/*Configure GPIO pin Output Level */
				HAL_GPIO_WritePin(GPIOC, DO_12VA_EN_Pin|DO_PAD_EN_Pin|DO_FAN_EN_Pin|DO_ACT_TOF1_SHUT_Pin, GPIO_PIN_RESET);
				/*Configure GPIO pin Output Level */
				HAL_GPIO_WritePin(GPIOE, DO_TOF2_SHUT_Pin|DO_AUDIO_SHDN_Pin|DO_I2C5_ADD_Pin, GPIO_PIN_RESET);
				/*Configure GPIO pin Output Level */
				HAL_GPIO_WritePin(GPIOD, DO_CTRL_LED2_Pin, GPIO_PIN_RESET);
			}
		}

		if(e_Key_Read_PWR() == _keyPRESSED){
			px_pub->u32_timToutRef = u32_Tim_1msGet();
		}

		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, MODE_LOWPWR_ENTRY_DELAY)){
			if(low_pwr == 0){
				low_pwr = 1;
				HAL_SuspendTick();
				__HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOP);
				HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

				if(__HAL_PWR_GET_FLAG(PWR_FLAG_STOP)){
					__HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOP);
					v_WakeUp_Clock_Config();
				}
				v_IO_PWR_WakeUp_Disable();
				v_Key_Power_Init();

				px_pub->u32_timToutRef = u32_Tim_1msGet();
			}
		}


	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
	}
}



/////////////////////////////////
//	MODE - WakeUp
/////////////////////////////////
// LOW: Removed duplicate declaration (already declared at line 1630)

void v_Mode_WakeUp_LED(){
	v_RGB_Set_Top(MODE_OFF_LED_R, MODE_OFF_LED_G, MODE_OFF_LED_B);
	v_RGB_Set_Bot(MODE_OFF_LED_R, MODE_OFF_LED_G, MODE_OFF_LED_B);
}

void v_Mode_WakeUp(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			v_Mode_Clear_PwrSW();
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			v_RTC_Write_BKUP(0xA5A5);
			v_IO_Enable_12V();
			v_RGB_Enable_Duty();
			v_Mode_WakeUp_LED();
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, MODE_WAKEUP_DELAY)){

			v_IO_Disable_12V();	//add : 1.00.34

			HAL_NVIC_SystemReset();
			v_Mode_MoveNext(px_work);
		}
	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
	}
}




/////////////////////////////////
//	MODE - Error
/////////////////////////////////

// 에러별 LED 5비트 패턴 정의
typedef struct {
	e_modeERR_t error_code;   // 에러 코드
	uint8_t led_pattern;      // 5비트 패턴 (0-31)
} s_ERROR_LED_CONFIG_t;

// 에러별 LED 패턴 테이블 (깜빡임 속도는 모두 500ms로 통일)
// LED 배치: [5][4][3][2][1] (왼쪽부터 오른쪽)
// 비트 순서: LED1=bit0(LSB), LED5=bit4(MSB)
static const s_ERROR_LED_CONFIG_t ERROR_LED_TABLE[] = {
	{modeERR_TEMP_IR,     0b00001},  // ○○○○● - 열화상 카메라
	{modeERR_TEMP_OUT,    0b00010},  // ○○○●○ - 실외 온도 센서
	{modeERR_TEMP_IN,     0b00011},  // ○○○●● - 실내 온도 센서
	{modeERR_TOF,         0b00100},  // ○○●○○ - 거리 센서
	{modeERR_IMU,         0b00101},  // ○○●○● - 자이로 센서
	{modeERR_FSR,         0b00110},  // ○○●●○ - 압력 센서
	{modeERR_AUDIO,       0b00111},  // ○○●●● - 오디오 모듈
	{modeERR_MP3_FILE,    0b01000},  // ○●○○○ - MP3 파일 없음
	{modeERR_SD_MOUNT,    0b01001},  // ○●○○● - SD 카드 오류
	{modeERR_BLOW_FAN,    0b01010},  // ○●○●○ - 송풍팬 고장
	{modeERR_COOL_FAN,    0b01011},  // ○●○●● - 쿨링팬 고장
	{modeERR_ESP_COMM,    0b01100},  // ○●●○○ - WiFi 통신 끊김
	{modeERR_FALL,        0b01101},  // ○●●○● - 낙상 감지
	{modeERR_HEATER_CURR, 0b11111},  // ●●●●● - 히터 과전류 (긴급!)
};
#define ERROR_LED_TABLE_SIZE (sizeof(ERROR_LED_TABLE) / sizeof(s_ERROR_LED_CONFIG_t))

// 에러 코드에 해당하는 LED 패턴 찾기
static uint8_t u8_Get_Error_LED_Pattern(e_modeERR_t err) {
	// 테이블에서 첫 번째 매칭되는 에러 찾기
	for(uint8_t i = 0; i < ERROR_LED_TABLE_SIZE; i++) {
		if(ERROR_LED_TABLE[i].error_code & err) {
			return ERROR_LED_TABLE[i].led_pattern;
		}
	}
	// 매칭 실패 시 기본 패턴 (모든 LED 켜짐)
	return 0b11111;
}

// 5비트 패턴을 10개 LED에 적용 (상단 5개 + 하단 5개 미러)
static void v_Set_Error_LED_Pattern(uint8_t pattern, bool blink_state) {
	uint8_t R = blink_state ? MODE_ERROR_LED_R : 0;
	uint8_t G = blink_state ? MODE_ERROR_LED_G : 0;
	uint8_t B = blink_state ? MODE_ERROR_LED_B : 0;

	// 5개 LED를 비트마스크에 따라 개별 제어
	for(uint8_t i = 0; i < 5; i++) {
		bool led_on = (pattern & (1 << i)) ? true : false;

		if(led_on) {
			// 이 LED는 패턴에 포함 → 깜빡임 상태에 따라 ON/OFF
			b_RGB_Set_Color(RGB_TOP_1 + i, R, G, B);
			b_RGB_Set_Color(RGB_BOT_1 + i, R, G, B);  // 하단 미러
		} else {
			// 이 LED는 패턴에 없음 → 항상 OFF
			b_RGB_Set_Color(RGB_TOP_1 + i, 0, 0, 0);
			b_RGB_Set_Color(RGB_BOT_1 + i, 0, 0, 0);
		}
	}

	// CRITICAL: Trigger RGB PWM output to SK6812 hardware
	v_RGB_Refresh_Enable();
}

static void v_Mode_Error_Led(uint16_t u16_toggle){
	//blink every 0.5 s using 5-bit pattern based on error type
	e_modeERR_t err = e_Mode_Get_Error();
	uint8_t pattern = u8_Get_Error_LED_Pattern(err);

	// 깜빡임 상태: 홀수=OFF, 짝수=ON
	bool blink_state = (u16_toggle & 1) ? false : true;

	// 패턴 적용
	v_Set_Error_LED_Pattern(pattern, blink_state);
}

static void v_Mode_Error(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static uint16_t ledToggle;
	static int sound;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;

		if(px_work->cr.bit.b1_on){
			//led
			px_pub->u32_timLedRef = u32_Tim_1msGet();
			px_pub->u32_timLedItv = 0;
			ledToggle = 0;
			v_RGB_Enable_Duty();  // Enable RGB PWM for error pattern display

			// ========== Actuator Safety Shutdown (Dual-Layer) ==========
			LOG_ERROR("MODE", "*** Actuator shutdown initiated ***");

			// 1st Layer: Software PWM shutdown
			v_Mode_Heater_Off();        // Heater PWM → 0, PID reset
			LOG_INFO("MODE", "  Heater: OFF (PWM=0, PID reset)");

			v_Mode_HeatPad_Disable();   // Heat pad PWM → 0
			LOG_INFO("MODE", "  Heat Pad: OFF (PWM=0)");

			v_Mode_BlowFan_Disable();   // Blow fan PWM → 0
			LOG_INFO("MODE", "  Blow Fan: OFF (PWM=0)");

			// 2nd Layer: Hardware enable pin shutdown
			HAL_GPIO_WritePin(DO_PAD_EN_GPIO_Port, DO_PAD_EN_Pin, GPIO_PIN_RESET);
			LOG_INFO("MODE", "  DO_PAD_EN: RESET (GPIO=LOW, hardware cutoff)");

			// ✅ Cooling fan remains active (residual heat removal)
			LOG_INFO("MODE", "  Cooling Fan: ON (residual heat removal)");
			LOG_ERROR("MODE", "*** Actuator shutdown complete ***");

			//sound
			if(i_Mode_Get_MP3_Play()){
				// MEDIUM: Stop any currently playing MP3 before error sound
				i_MP3_ForceStop();
				sound = 1;
			}
			//debug
#if MODE_LOG_ENABLED
			LOG_DEBUG("MODE", "error start.");
			LOG_DEBUG("MODE", "prev mode : %d", px_work->guide.e_prev);
#endif
			// Don't send error to ESP - ESP gets error info from sensing data (imu_evt)
			// v_ESP_Send_EvtModeChange(ESP_EVT_MODE_ERROR);
			// v_ESP_Send_Error((uint16_t)e_Mode_Get_Error());
		}
		else{
			px_work->cr.bit.b1_off = 0;
			px_work->cr.bit.b1_on = 1;
		}
	}
	if(px_work->cr.bit.b1_on){
		//only reboot
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timLedRef, px_pub->u32_timLedItv)){
			px_pub->u32_timLedRef = u32_Tim_1msGet();
			px_pub->u32_timLedItv = MODE_LED_BLINK_ITV;

			v_Mode_Error_Led(ledToggle);
			ledToggle++;
			// LOW: Removed unused commented code
		}

		if(sound){
			// 코덱이 아직 초기화 안 됐으면 초기화 계속 진행
			if(e_Codec_Ready() != COMM_STAT_DONE){
				return;  // 다음 사이클에서 재시도
			}

			e_modeERR_t err = e_Mode_Get_Error();
			uint16_t mp3=0;
			// LOW: Use named constants instead of magic numbers
			if(err & modeERR_TEMP_IR)		{mp3=MODE_MP3_ERROR_TEMP_IR;}
			else if(err & modeERR_TEMP_OUT)	{mp3=MODE_MP3_ERROR_TEMP_OUT;}
			else if(err & modeERR_IMU)		{mp3=MODE_MP3_ERROR_IMU;}
			else if(err & modeERR_BLOW_FAN)	{mp3=MODE_MP3_ERROR_BLOW_FAN;}
			else if(err & modeERR_COOL_FAN)	{mp3=MODE_MP3_ERROR_COOL_FAN;}
			else if(err & modeERR_TOF)		{mp3=MODE_MP3_ERROR_TOF;}

			if(mp3 != 0){
				if(i_MP3_Is_Ready()){
					i_MP3_Begin(mp3);
					sound = 0;
				}
			}
		}
	}
	else if(px_work->cr.bit.b1_off){
		//v_Mode_MoveNext(px_work);
	}
}


/////////////////////////////////
//	MODE - TEST (JIG)
/////////////////////////////////
static float f_dbg_temp_max;
static int i_dbg_fan_speed;  // LOW: Fixed typo (spped -> speed)
static float f_dbg_temp_forceUp;
static float f_dbg_temp_forceDown;
static float f_dbg_temp_waiting;
static uint32_t u32_dbg_tout;
static int i_modeTest;

void v_Mode_DBG_Enter(){
	v_Mode_SetNext(modeTEST);
}

void v_Mode_Set_DBG_TempMax(float f_max){
	f_dbg_temp_max = f_max;
}

void v_Mode_Set_DBG_FanSpeed(int i_speed){
	i_dbg_fan_speed = i_speed;
}

void v_Mode_Set_DBG_FoceUp(float f_temp){
	if(f_temp <= f_dbg_temp_max){
		f_dbg_temp_forceUp = f_temp;
	}
	else{
		f_dbg_temp_forceUp = f_dbg_temp_max;
	}
}

void v_Mode_Set_DBG_ForceDown(float f_temp){
	f_dbg_temp_forceDown = f_temp;
}

void v_Mode_Set_DBG_Waiting(float f_temp){
	f_dbg_temp_waiting = f_temp;
}

void v_Mode_Set_DBG_Tout(uint32_t u32_time){
	u32_dbg_tout = u32_time;
}

void v_Mode_Set_DBG_Act(int type){
	int mode = modeTEST_IDLE;
	if(type <= modeTEST_WAITING){
		mode = type;
	}
	i_modeTest = mode | modeTEST_ACT;
}


static void v_Mode_TEST_Led(int i_toggle){
	if(i_toggle & 1){
		v_RGB_Set_Top(0, 0, 0);
		v_RGB_Set_Bot(0, 0, 0);
	}
	else{
		//on
		v_RGB_Set_Top(MODE_TEST_LED_R, MODE_TEST_LED_G, MODE_TEST_LED_B);
		v_RGB_Set_Bot(MODE_TEST_LED_R, MODE_TEST_LED_G, MODE_TEST_LED_B);
	}
}

static void v_Mode_TEST(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){
	if(e_id != px_work->guide.e_curr){return;}
	static int32_t data[12];
	static uint16_t ledToggle;
	static int action;
	static int cplt;
	static float tempTarget;
	if(px_work->cr.bit.b1_upd){
		px_work->cr.bit.b1_upd = 0;
		if(px_work->cr.bit.b1_on){
			px_pub->u32_timLedRef = u32_Tim_1msGet();
			px_pub->u32_timLedItv = 0;

			v_ESP_Send_EvtModeChange(ESP_EVT_MODE_TEST);
			//LED
			ledToggle = 0;
		}
		else{

		}
	}
	if(px_work->cr.bit.b1_on){
		if(i_modeTest & modeTEST_ACT){
			i_modeTest &= ~modeTEST_ACT;
			action =  i_modeTest;
			//temperature condition renew
			px_pub->i_tempRenew = 1;
			cplt = 0;
			v_Mode_Heater_Off();
			v_Mode_CoolFan_Disable();
			px_pub->u32_timToutRef = u32_Tim_1msGet();
			if(action == modeTEST_FORCE_UP){
				tempTarget = f_dbg_temp_forceUp;
			}
			else if(action == modeTEST_FORCE_DOWN){
				tempTarget = f_dbg_temp_forceDown;
			}
			else if(action == modeTEST_WAITING){
				tempTarget = f_dbg_temp_waiting;
			}
		}

		if(action == modeTEST_FORCE_UP){
			if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, u32_dbg_tout)){
				if(!cplt){
					cplt = 1;
					v_DBG_Transmit(0x0B, data, 0);
					tempTarget = f_IR_Temp_Get();
				}
			}

			int temp = i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, tempTarget);
			if(temp > 0){
				v_Mode_Heater_Off();
				if(!cplt){
					cplt = 1;
					v_DBG_Transmit(0x0B, data, 0);
				}
			}
			else if(temp < 0){
				if(e_Mode_Get_Error() & modeERR_TEMP_IR){
					v_Mode_Heater_Off();
				}
				else{
					f_Mode_Heater_PID_Handler(tempTarget);
				}
			}
		}
		else if(action == modeTEST_FORCE_DOWN){
			if(cplt){
				//keep temperature
				if(i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, tempTarget) == 1){
					v_Mode_Heater_Off();
				}
				else{
					if(e_Mode_Get_Error() & modeERR_TEMP_IR){
						v_Mode_Heater_Off();
					}
					else{
						f_Mode_Heater_PID_Handler(tempTarget);
					}
				}
			}
			else{
				v_Mode_CoolFan_Enable();

				if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, u32_dbg_tout)){
					cplt = 1;
					v_Mode_CoolFan_Disable();
					v_DBG_Transmit(0x0D, data, 0);
					tempTarget = f_IR_Temp_Get();
				}
				else{
					if(i_Mode_Is_TempHeater_Under(&px_pub->i_tempRenew, tempTarget) == 1){
						cplt = 1;
						v_Mode_CoolFan_Disable();
						v_DBG_Transmit(0x0D, data, 0);
					}
				}
			}
		}
		else if(action == modeTEST_WAITING){
			if(cplt){
				//keep temperature
				if(i_Mode_Is_TempHeater_Over(&px_pub->i_tempRenew, tempTarget) == 1){
					v_Mode_Heater_Off();
				}
				else{
					if(e_Mode_Get_Error() & modeERR_TEMP_IR){
						v_Mode_Heater_Off();
					}
					else{
						f_Mode_Heater_PID_Handler(tempTarget);
					}
				}
			}
			else{
				v_Mode_CoolFan_Enable();

				if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timToutRef, u32_dbg_tout)){
					cplt = 1;
					v_Mode_CoolFan_Disable();
					v_DBG_Transmit(0x0C, data, 0);
					tempTarget = f_IR_Temp_Get();
				}
				else{
					if(i_Mode_Is_TempHeater_Under(&px_pub->i_tempRenew, tempTarget) == 1){
						cplt = 1;
						v_Mode_CoolFan_Disable();
						v_DBG_Transmit(0x0C, data, 0);
					}
				}
			}
		}
		else{
			//nothing
		}
	}
	else if(px_work->cr.bit.b1_off){
		v_Mode_MoveNext(px_work);
		v_Mode_CoolFan_Disable();
		v_Mode_Heater_Off();
	}

	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), px_pub->u32_timLedRef, px_pub->u32_timLedItv)){
		px_pub->u32_timLedRef = u32_Tim_1msGet();
		px_pub->u32_timLedItv = MODE_LED_BLINK_ITV;

		v_Mode_TEST_Led(ledToggle++);
		// LOW: Removed unused commented code
	}

	v_Mode_Sensing_Handler();
	v_Mode_BlowFan_Handler();
	v_Mode_CoolFan_Handler();
#if HEAT_PAD_USED
	v_Mode_HeatPad_Handler();
#endif
	v_Mode_Bat_Handler();
}




/////////////////////////////////
//	Mode - Handler
/////////////////////////////////
void v_Mode_Init(){
	v_TOF_Deinit();
	v_TempIR_Deinit();
	v_FSR_Deinit();
	v_IMU_Deinit();
	v_Codec_Deinit();
	v_Temp_InOut_Deinit();

	v_Mode_Heater_PID_Init();
#if MODE_TEST_SUB_BD
	v_Mode_Set_AI(MODE_AI_ESP);
	v_Mode_SetInit(modeBOOTING);
#else
	v_Mode_Set_AI(MODE_AI_ESP);
	if(u32_RTC_Read_BKUP() == 0xA5A5){
		v_RTC_Write_BKUP(0x0000);
		v_Mode_SetInit(modeBOOTING);
	}
	else{
		LOG_INFO("MODE", "Cold boot detected, RTC_backup=0x%08lX, starting in modeOFF", u32_RTC_Read_BKUP());
		v_Mode_SetInit(modeOFF);
	}
#endif

	v_IO_Enable_Fan();			//always	//25.07.10
	v_Mode_Bat_Alarm_Enable();	//once play


	//value temporary
	v_Mode_Config_Tout(MODE_TOUT_WAITING, MODE_TOUT_FORCE_UP, MODE_TOUT_FORCE_ON, MODE_TOUT_FORCE_DOWN);
	v_Mode_Set_Temp_Max(MODE_TEMP_MAX);
	v_Mode_Set_Temp_ForceUp(MODE_TEMP_FORCE_UP);
	v_Mode_Set_Temp_Waiting(MODE_TEMP_WAITING);
	v_Mode_Set_Temp_Sleep(MODE_TEMP_SLEEP);

	//cool fan
	v_Mode_Set_CoolFan_Now(MODE_COOL_FAN_STEP_INIT);
	v_Mode_Set_CoolFan_Max(MODE_COOL_FAN_STEP_MAX);
	v_Mode_CoolFan_Disable();
	//heat pad
	v_Mode_Set_HeatPad_Now(MODE_HEATPAD_STEP_INIT);
	v_Mode_Set_HeatPad_Max(MODE_HEATPAD_STEP_MAX);
	v_Mode_HeatPad_Disable();

	//blow fan
	v_Mode_Set_BlowFan_Now(MODE_BLOWFAN_STEP_INIT);
	v_Mode_Set_BlowFan_Max(MODE_BLOWFAN_STEP_MAX);
	v_Mode_BlowFan_Enable();

	//forceDown Delay
	v_Mode_Set_ForceDownDelay(MODE_FORCE_DOWN_COOL_DELAY_INIT);

	//spkear
	//v_Mode_Set_Speaker_Vol(1);

	//gyro angle
	v_Mode_Set_GyroAngle_Act(MODE_GYRO_ANGLE_ACT_INIT);
	v_Mode_Set_GyroAngle_Rel(MODE_GYRO_ANGLE_REL_INIT);
}

void v_Mode_Handler(){
	v_Mode_Booting(modeBOOTING, &x_modeWork, &x_modePub);
	v_Mode_Healing(modeHEALING, &x_modeWork, &x_modePub);
	v_Mode_Waiting(modeWAITING, &x_modeWork, &x_modePub);
	v_Mode_ForceUp(modeFORCE_UP, &x_modeWork, &x_modePub);
	v_Mode_ForceOn(modeFORCE_ON, &x_modeWork, &x_modePub);
	v_Mode_ForceDown(modeFORCE_DOWN, &x_modeWork, &x_modePub);
	v_Mode_Sleep(modeSLEEP, &x_modeWork, &x_modePub);
	v_Mode_Off(modeOFF, &x_modeWork, &x_modePub);
	v_Mode_Error(modeERROR, &x_modeWork, &x_modePub);
	v_Mode_TEST(modeTEST, &x_modeWork, &x_modePub);
	v_Mode_WakeUp(modeWAKE_UP, &x_modeWork, &x_modePub);

	if(i_Mode_Get_MP3_Play()){i_MP3_Playing();}

	if(i_Mode_Is_Off()){return;}
	//tout
	v_FSR_Tout_Handler();
	v_IMU_Tout_Handler();
	v_Codec_Tout_Handler();
	v_Temp_InOut_Tout_Handler();
	v_Temp_IR_Tout_Handler();
	v_ESP_Tout_Handler();
	//spkear
	v_Mode_Speaker_Vol_Handler();
	i_Mode_TempHeater_FB();

#if MODE_TEST_SUB_BD
	//bd test
	if(x_modeWork.guide.e_curr > modeBOOTING){
		v_Mode_SubBD_Print();
	}
#endif
}





/////////////////////////////////
//	Only Test
/////////////////////////////////
void v_Mode_CoolFan(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, MODE_COOLFAN_HANDLER_ITV)){
		timRef = u32_Tim_1msGet();
		v_Mode_CoolFan_Enable();
		v_Mode_CoolFan_Handler();
		// LOW: Removed unused commented code
	}
}

void v_Mode_Sound_Test(){
	static int config;
	static uint32_t timRef;
	static int play;
	if(config == 0){
		if(e_Codec_Ready() == COMM_STAT_DONE){
			config = 1;
		}
	}
	else{
		int volume = i_Mode_Get_Speaker_Vol();
		if(i_Mode_Get_BlowFanSW()){
			v_Mode_Clear_BlowFanSW();
			if(volume){volume--;}
			v_Mode_Set_Speaker_Vol(volume);

			if(i_MP3_Is_Ready()){
				timRef = u32_Tim_1msGet();
				play = 1;
			}
		}
		if(i_Mode_Get_ToggleSw()){
			v_Mode_Clear_ToggleSW();
			if(volume < MODE_SPEAKER_VOL_MAX){volume++;}
			v_Mode_Set_Speaker_Vol(volume);

			if(i_MP3_Is_Ready()){
				timRef = u32_Tim_1msGet();
				play = 1;
			}
		}
		v_Mode_Speaker_Vol_Handler();


		if(play && _b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, MODE_SOUND_TEST_DELAY)){
			if(i_MP3_Player(1) == MP3_DONE){
				play = 0;
			}
		}
	}
}



__attribute__((unused)) static void v_Mode_SubBD_Print(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, MODE_SUBBD_PRINT_ITV)){
		timRef = u32_Tim_1msGet();
		int16_t* imu_L = pi16_IMU_Get_Left();
		int16_t* imu_R = pi16_IMU_Get_Right();
		(void)imu_L; (void)imu_R;  // Suppress warnings when logs disabled
		LOG_DEBUG("MODE", "[L] ACC - X : %-7d, Y : %-7d, Z : %-7d / GYRO - X : %-7d, Y : %-7d, Z : %-7d", imu_L[0], imu_L[1], imu_L[2], imu_L[3], imu_L[4], imu_L[5]);
		LOG_DEBUG("MODE", "[R] ACC - X : %-7d, Y : %-7d, Z : %-7d / GYRO - X : %-7d, Y : %-7d, Z : %-7d", imu_R[0], imu_R[1], imu_R[2], imu_R[3], imu_R[4], imu_R[5]);
		LOG_DEBUG("MODE", "Temp : %.2f", f_Temp_Out_Get());
	}
}

// 에러 LED 패턴 테스트 함수 (시리얼 명령어로 호출용)
void v_Mode_Error_LED_Test(e_modeERR_t test_error) {
	LOG_INFO("MODE", "Testing error pattern for code: 0x%04X", test_error);

	// 에러 강제 설정
	e_modeError = test_error;
	v_Mode_SetNext(modeERROR);

	// 예상 패턴 출력
	uint8_t pattern = u8_Get_Error_LED_Pattern(test_error);
	(void)pattern;  // Suppress warning when logs disabled
	LOG_INFO("MODE", "  LED Pattern (5-bit): 0b%c%c%c%c%c",
	       (pattern & 0b10000) ? '1' : '0',
	       (pattern & 0b01000) ? '1' : '0',
	       (pattern & 0b00100) ? '1' : '0',
	       (pattern & 0b00010) ? '1' : '0',
	       (pattern & 0b00001) ? '1' : '0');
	LOG_INFO("MODE", "  Visual: [%c][%c][%c][%c][%c]",
	       (pattern & 0b10000) ? 'O' : 'X',
	       (pattern & 0b01000) ? 'O' : 'X',
	       (pattern & 0b00100) ? 'O' : 'X',
	       (pattern & 0b00010) ? 'O' : 'X',
	       (pattern & 0b00001) ? 'O' : 'X');
}

