#include "comm_esp.h"
#include "lib_ringbuf.h"
#include "stdio.h"
#include "mode.h"
#include "tim.h"
#include "math.h"


//STX	| LEN	| DIR	| CMD	| DATA	| CHK	| ETX
#define ESP_FMT_STX			(0x02)
#define ESP_FMT_ETX			(0x03)
#define ESP_FMT_SIZE_MIN	(6)
#define ESP_FMT_LEN_MIN		(ESP_FMT_SIZE_MIN - 2)
#define ESP_FMT_CHK_INIT	(0xA5)


#define ESP_TEST_MODE	1

typedef enum {
	ESP_DIR_ACK=0x02,
	ESP_DIR_REQ=0x20,
} ESP_DIR_t;

typedef enum {
	//INITIALIZE
	ESP_CMD_INIT_TEMP_SLEEP		=0x10,
	ESP_CMD_INIT_TEMP_WAITING	=0x11,
	ESP_CMD_INIT_TEMP_FORCEUP	=0x12,
	ESP_CMD_INIT_TEMP_LIMIT		=0x14,
	ESP_CMD_INIT_PWM_COOLFAN	=0x15,
	ESP_CMD_INIT_TOUT			=0x16,
	ESP_CMD_INIT_SPK			=0x17,
	ESP_CMD_INIT_DELAY			=0x18,
	ESP_CMD_INIT_GYRO_ACT		=0x19,
	ESP_CMD_INIT_GYRO_REL		=0x20,
	ESP_CMD_INIT_MODE			=0x21,
	ESP_CMD_INIT_PWM_BLOWFAN	=0x22,
	//REQUEST
	ESP_CMD_REQ_TEMP_SLEEP		=0x30,
	ESP_CMD_REQ_TEMP_WAITING	=0x31,
	ESP_CMD_REQ_TEMP_FORCEUP	=0x32,
	ESP_CMD_REQ_PWM_HEATPAD		=0x33,
	ESP_CMD_REQ_TEMP_LIMIT		=0x34,
	ESP_CMD_REQ_PWM_COOLFAN		=0x35,
	ESP_CMD_REQ_TOUT			=0x36,
	ESP_CMD_REQ_SPK				=0x37,
	ESP_CMD_REQ_DELAY			=0x38,
	ESP_CMD_REQ_GYRO_ACT		=0x39,
	ESP_CMD_REQ_GYRO_REL		=0x40,
	ESP_CMD_REQ_MODE			=0x41,
	ESP_CMD_REQ_PWM_BLOWFAN		=0x42,
	//CONTROL
	ESP_CMD_CTRL_RST			=0x50,
	ESP_CMD_CTRL_MODE			=0x51,	//add
	ESP_CMD_CTRL_SPK_ON			=0x52,
	ESP_CMD_CTRL_BLOWFAN_ON		=0x53,
	ESP_CMD_CTRL_COOLFAN_ON		=0x55,
	//STATUS
	ESP_CMD_STAT=0x70,
	//EVENT
	ESP_CMD_EVT_INIT_START=0x80,
    ESP_CMD_EVT_INIT_RESULT=0x81,
    ESP_CMD_EVT_MODE=0x82,
	//ERROR
	ESP_CMD_ERR=0x90,
} e_ESP_CMD_t;






#define ESP_RX_ARR_SIZE		(128)


//function
static void v_ESP_Transmit(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len);
static void v_ESP_RxHandler();
static void v_ESP_RxProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_RxAck(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);

static void v_ESP_InitProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_ReqProc(uint8_t u8_cmd);
static void v_ESP_CtrlProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_StatProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);

//vairlabe
_RING_VAR_DEF(espRx, uint8_t, ESP_RX_ARR_SIZE);
static uint32_t u32_toutRef;
static int i_toutAct;





void v_ESP_Handler(){
	v_ESP_RxHandler();
}

/*
 * brief	: esp receive
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void v_ESP_Recive(uint8_t u8_rx){
	espRx->fn.v_Put(espRx, u8_rx);
}

/*
 * brief	: esp format create
 * date
 * - create	: 25.04.28
 * - modify	: 25.04.29
 * param
 * - u8_dir		: direction (req or ack)
 * - u8_cmd		: command
 * - pu8_data	: data array
 * - u16_len	: data length
 */
static void v_ESP_Transmit(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len){
	// CRITICAL: Validate buffer size to prevent stack overflow
	// fmt[64] = STX(1) + LEN(1) + DIR(1) + CMD(1) + DATA(u16_len) + CHK(1) + ETX(1)
	// Maximum safe data length: 64 - 6 = 58 bytes
	if(u16_len > (ESP_TX_FMT_BUF_SIZE - 6)){
		return;  // Prevent buffer overflow
	}

	uint8_t fmt[ESP_TX_FMT_BUF_SIZE];
	//uint8_t fmt[ESP_FMT_SIZE_MIN + u16_len + 1];
	uint8_t chk = ESP_FMT_CHK_INIT;
	uint16_t cnt=0;

	//STX
	fmt[cnt] = ESP_FMT_STX;
	chk ^= fmt[cnt++];
	//LEN
	fmt[cnt] = ESP_FMT_LEN_MIN + u16_len;
	chk ^= fmt[cnt++];
	//DIR
	fmt[cnt] = u8_dir;
	chk ^= fmt[cnt++];
	//CMD
	fmt[cnt] = u8_cmd;
	chk ^= fmt[cnt++];
	//DATA
	for(uint16_t i=0; i<u16_len; i++){
		fmt[cnt] = pu8_data[i];
		chk ^= fmt[cnt++];
	}
	//CHK
	fmt[cnt++] = chk;
	//ETX
	fmt[cnt++] = ESP_FMT_ETX;	//etx

	v_Uart_ESP_Out(fmt, cnt);
}

/*
 * brief	: command compare
 * date
 * - create	: 25.04.28
 * - modify	: 25.04.29
 */
bool b_ESP_CmdCompare(uint8_t u8_cmd){
	if(u8_cmd == ESP_CMD_STAT\
	||(u8_cmd >= ESP_CMD_INIT_TEMP_SLEEP && u8_cmd <= ESP_CMD_INIT_PWM_BLOWFAN)\
	||(u8_cmd >= ESP_CMD_REQ_TEMP_SLEEP && u8_cmd <= ESP_CMD_REQ_PWM_BLOWFAN)\
	||(u8_cmd >= ESP_CMD_CTRL_RST && u8_cmd <= ESP_CMD_CTRL_COOLFAN_ON)\
	||(u8_cmd >= ESP_CMD_EVT_INIT_START && u8_cmd <= ESP_CMD_EVT_MODE)\
	||(u8_cmd == ESP_CMD_ERR)){
		return true;
	}
	else{
		return false;
	}
}


/*
 * brief	: receive handler
 * date
 * - create	: 25.04.28
 * - modify	: 25.04.29
 */
static void v_ESP_RxHandler(){
	static uint8_t len;
	//recursive check
	if(espRx->u16_cnt < ESP_FMT_SIZE_MIN + len){return;}
	//	format compare	//
	uint8_t chk = ESP_FMT_CHK_INIT;
	uint16_t out = espRx->u16_out;
	uint16_t mask = espRx->u16_mask;
	uint8_t* p_arr = espRx->p_arr;
	uint8_t data_len;
	uint8_t cmd, dir;
	uint8_t data[32];

	//STX
	if(p_arr[out] != ESP_FMT_STX){
		v_Uart_ESP_DisableIT();
		espRx->fn.b_Jmp(espRx, 1);
		v_Uart_ESP_EnableIT();
		len = 0;
		return;
	}
	chk ^= p_arr[out];
	out = (out + 1) & mask;
	//LEN
	len = p_arr[out];
	if(len < ESP_FMT_LEN_MIN || len > mask){
		v_Uart_ESP_DisableIT();
		espRx->fn.b_Jmp(espRx, 1);
		v_Uart_ESP_EnableIT();
		len = 0;
		return;
	}
	len -= ESP_FMT_LEN_MIN;
	if(ESP_FMT_SIZE_MIN + len > espRx->u16_cnt){
		return;
	}
	chk ^= p_arr[out];
	out = (out + 1) & mask;
	data_len = len;
	// CRITICAL FIX: Validate data_len to prevent buffer overflow
	if(data_len > 32){
		v_Uart_ESP_DisableIT();
		espRx->fn.b_Jmp(espRx, 1);
		v_Uart_ESP_EnableIT();
		len = 0;
		return;
	}
	//DIR
	dir = p_arr[out];
	chk ^= p_arr[out];
	out = (out + 1) & mask;
	//CMD
	cmd = p_arr[out];
	if(b_ESP_CmdCompare(cmd) == false){
		v_Uart_ESP_DisableIT();
		espRx->fn.b_Jmp(espRx, 1);
		v_Uart_ESP_EnableIT();
		len = 0;
		return;
	}
	chk ^= p_arr[out];
	out = (out + 1) & mask;
	//DATA
	for(uint16_t i=0; i<data_len; i++){
		data[i] = p_arr[out];
		chk ^= p_arr[out];
		out = (out + 1) & mask;
	}
	//CHK
	if(chk != p_arr[out]){
		v_Uart_ESP_DisableIT();
		espRx->fn.b_Jmp(espRx, 1);
		v_Uart_ESP_EnableIT();
		len = 0;
		return;
	}
	out = (out + 1) & mask;
	//ETX
	if(p_arr[out] != ESP_FMT_ETX){
		v_Uart_ESP_DisableIT();
		espRx->fn.b_Jmp(espRx, 1);
		v_Uart_ESP_EnableIT();
		len = 0;
		return;
	}
	//decrease count..
	v_Uart_ESP_DisableIT();
	espRx->fn.b_Jmp(espRx, ESP_FMT_SIZE_MIN + len);
	v_Uart_ESP_EnableIT();
	len = 0;
	//receive process
	if(dir == ESP_DIR_REQ){
		v_ESP_RxProc(cmd, data, data_len);
	}
	else{
		v_ESP_RxAck(cmd, data, data_len);
	}
}


/*
 * brief	: receive protocol process
 * date
 * - create	: 25.04.29
 * - modify	: -
 */
static void v_ESP_RxProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	if(u8_cmd >= ESP_CMD_CTRL_RST && u8_cmd <= ESP_CMD_CTRL_COOLFAN_ON){
		//CTRL
		v_ESP_CtrlProc(u8_cmd, pu8_data, u8_len);
	}
	else if(u8_cmd >= ESP_CMD_INIT_TEMP_SLEEP && u8_cmd <= ESP_CMD_INIT_PWM_BLOWFAN){
		//INIT
		v_ESP_InitProc(u8_cmd, pu8_data, u8_len);
	}
	else if(u8_cmd >= ESP_CMD_REQ_TEMP_SLEEP && u8_cmd <= ESP_CMD_REQ_PWM_BLOWFAN){
		//REQ
		v_ESP_ReqProc(u8_cmd);
	}
}

static void v_ESP_RxAck(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	if(u8_cmd == ESP_CMD_STAT){
		v_ESP_StatProc(u8_cmd, pu8_data, u8_len);
	}
}

e_ESP_CMD_t in_cmd;
/*
 * brief	: command-init process
 * date
 * - create	: 25.04.29
 * - modify	: -
 * param
 * - u8_cmd		: command
 * - pu8_data	: data array
 * - u8_len		: data array length
 * note
 * - parameter update
 */
static void v_ESP_InitProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	float typ;
	in_cmd = u8_cmd;

	switch(u8_cmd){
	case ESP_CMD_INIT_TEMP_SLEEP:
		typ = pu8_data[0];
		if(pu8_data[1] < 10){
			typ += (pu8_data[1] / 10.0);
		}
		v_Mode_Set_Temp_Sleep(typ);
		break;
	case ESP_CMD_INIT_TEMP_WAITING:
		typ = pu8_data[0];
		if(pu8_data[1] < 10){
			typ += (pu8_data[1] / 10.0);
		}
		v_Mode_Set_Temp_Waiting(typ);
		break;
	case ESP_CMD_INIT_TEMP_FORCEUP:
		typ = pu8_data[0];
		if(pu8_data[1] < 10){
			typ += (pu8_data[1] / 10.0);
		}
		v_Mode_Set_Temp_ForceUp(typ);
		break;
	case ESP_CMD_INIT_TEMP_LIMIT:
		typ = pu8_data[0];
		if(pu8_data[1] < 10){
			typ += (pu8_data[1] / 10.0);
		}
		v_Mode_Set_Temp_Max(typ);
		break;
	case ESP_CMD_INIT_PWM_COOLFAN:
		v_Mode_Set_CoolFan_Now(pu8_data[0]);
		v_Mode_Set_CoolFan_Max(pu8_data[1]);
		break;
	case ESP_CMD_INIT_TOUT:
	{
		uint32_t waiting, forceUp, forceOn, forceDown;
		forceUp = (pu8_data[0] << 8) | (pu8_data[1] & 0xFF);
		forceOn = (pu8_data[2] << 8) | (pu8_data[3] & 0xFF);
		forceDown = (pu8_data[4] << 8) | (pu8_data[5] & 0xFF);
		waiting = (pu8_data[6] << 8) | (pu8_data[7] & 0xFF);
		v_Mode_Config_Tout(waiting, forceUp, forceOn, forceDown);
	}
		break;
	case ESP_CMD_INIT_SPK:
		v_Mode_Set_Speaker_Vol(pu8_data[0]);
		break;
	case ESP_CMD_INIT_DELAY:
		v_Mode_Set_ForceDownDelay(pu8_data[0]);
		break;
	case ESP_CMD_INIT_GYRO_ACT:
		v_Mode_Set_GyroAngle_Act(pu8_data[0]);
		break;
	case ESP_CMD_INIT_GYRO_REL:
		v_Mode_Set_GyroAngle_Rel(pu8_data[0]);
		break;
	case ESP_CMD_INIT_MODE:
		v_Mode_Set_AI(pu8_data[0]);
		break;
	case ESP_CMD_INIT_PWM_BLOWFAN:
		v_Mode_Set_BlowFan_Now(pu8_data[0]);
		break;
	}
	//ack
	v_ESP_Transmit(ESP_DIR_ACK, u8_cmd, NULL, 0);
}


/*
 * brief	: command-req process
 * date
 * - create	: 25.04.29
 * - modify	: -
 * prama
 * - u8_cmd	: command
 * note
 * - response parameter
 */
static void v_ESP_ReqProc(uint8_t u8_cmd){
	static uint8_t data[32];
	float temp;
	double integ, dot;
	uint16_t len=0;
	switch(u8_cmd){
	case ESP_CMD_REQ_TEMP_SLEEP:
		temp = f_Mode_Get_Temp_Sleep();
		dot = modf(temp, &integ);
		data[len++] = integ;
		data[len++] = dot * 10;
		break;
	case ESP_CMD_REQ_TEMP_WAITING:
		temp = f_Mode_Get_Temp_Waiting();
		dot = modf(temp, &integ);
		data[len++] = integ;
		data[len++] = dot * 10;
		break;
	case ESP_CMD_REQ_TEMP_FORCEUP:
		temp = f_Mode_Get_Temp_ForceUp();
		dot = modf(temp, &integ);
		data[len++] = integ;
		data[len++] = dot * 10;
		break;
	case ESP_CMD_REQ_PWM_HEATPAD:
		data[len++] = u16_Mode_Get_HeatPad_Now();
		data[len++] = u16_Mode_Get_HeatPad_Max();
		break;
	case ESP_CMD_REQ_TEMP_LIMIT:
		temp = f_Mode_Get_Temp_Max();
		dot = modf(temp, &integ);
		data[len++] = integ;
		data[len++] = dot * 10;
		break;
	case ESP_CMD_REQ_PWM_COOLFAN:
		data[len++] = u16_Mode_Get_CoolFan_Now();
		data[len++] = u16_Mode_Get_CoolFan_Max();
		break;
	case ESP_CMD_REQ_TOUT:
	{
		uint16_t tout;
		tout = u32_Mode_Get_ForceUp_Tout();
		data[len++] = tout >> 8;
		data[len++] = tout;
		tout = u32_Mode_Get_ForceOn_Tout();
		data[len++] = tout >> 8;
		data[len++] = tout;
		tout = u32_Mode_Get_ForceDown_Tout();
		data[len++] = tout >> 8;
		data[len++] = tout;
		tout = u32_Mode_Get_Waiting_Tout();
		data[len++] = tout >> 8;
		data[len++] = tout;
		break;
	}
	case ESP_CMD_REQ_SPK:
		data[len++] = i_Mode_Get_Speaker_Vol();
		break;
	case ESP_CMD_REQ_DELAY:
		data[len++] = u32_Mode_Get_ForceDownDelay();
		break;
	case ESP_CMD_REQ_GYRO_ACT:
		data[len++] = i_Mode_Get_GyroAngle_Act();
		break;
	case ESP_CMD_REQ_GYRO_REL:
		data[len++] = i_Mode_Get_GyroAngle_Rel();
		break;
	case ESP_CMD_REQ_MODE:
		data[len++] = i_Mode_Get_AI();
		break;
	case ESP_CMD_REQ_PWM_BLOWFAN:
		data[len++] = u16_Mode_Get_BlowFan_Now();
		break;
	}
	//ack
	v_ESP_Transmit(ESP_DIR_ACK, u8_cmd, data, len);
}


/*
 * brief	: command-req process
 * date
 * - create	: 25.04.29
 * - modify	: -
 * param
 * - u8_cmd		: command
 * - pu8_data	: data array
 * - u8_len		: data array length
 * note
 * - The STM is controlled by the ESP.
 */
static void v_ESP_CtrlProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	switch(u8_cmd){
	case ESP_CMD_CTRL_RST:
		//NVIC Reset
		v_Mode_SetNext(modeOFF);
		break;
	case ESP_CMD_CTRL_MODE:{
		//mode convert
		uint8_t mode = pu8_data[0];
		if(mode == 0)		{v_Mode_SetNext(modeSLEEP);}
		else if(mode == 1)	{v_Mode_SetNext(modeWAITING);}
		else if(mode == 2)	{v_Mode_SetNext(modeFORCE_UP);}
		else if(mode == 3)	{v_Mode_SetNext(modeFORCE_ON);}
		else if(mode == 4)	{v_Mode_SetNext(modeFORCE_DOWN);}
		break;
	}
	case ESP_CMD_CTRL_SPK_ON:
		//0	: mute
		//1	: on
		if(pu8_data[0] == 0)		{v_Mode_Set_Speaker_Mute(1);}
		else if(pu8_data[0] == 1)	{v_Mode_Set_Speaker_Mute(0);}
		break;
	case ESP_CMD_CTRL_BLOWFAN_ON:
		if(pu8_data[0] == 1){
			v_Mode_BlowFan_Enable();
		}
		else{
			v_Mode_BlowFan_Disable();
		}
		break;
	case ESP_CMD_CTRL_COOLFAN_ON:
		if(pu8_data[0] == 1){
			v_Mode_CoolFan_Enable();
		}
		else{
			v_Mode_CoolFan_Disable();
		}
		break;
	}
	//ack
	v_ESP_Transmit(ESP_DIR_ACK, u8_cmd, NULL, 0);
}


static void v_ESP_StatProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	if(u8_cmd == ESP_CMD_STAT){
		i_toutAct = 0;
	}
}

void v_ESP_Send_InitStart(){
	v_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_INIT_START, NULL, 0);
}

void v_ESP_Send_InitEnd(){
	v_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_INIT_RESULT, NULL, 0);
}


/*
 * brief	: event - mode change (stm -> esp)
 * date
 * - create	: 25.05.07
 * - modify	: -
 */
void v_ESP_Send_EvtModeChange(uint8_t u8_mode){
	v_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_MODE, &u8_mode, 1);
}

void v_ESP_Send_Sensing(int16_t* pi16_imu_left, int16_t* pi16_imu_right,\
						uint16_t u16_fsr_left, uint16_t u16_fsr_right,\
						float f_tempOut, float f_tempIn, float f_tempIR,\
						uint16_t u16_tof1, uint16_t u16_tof2, float f_bat,\
						uint8_t u8_imu_left_evt, uint8_t u8_imu_right_evt){
	uint8_t data[64] = {0,};
	uint16_t cnt=0;
	uint16_t idx;
	float temp;
	//	IMU LEFT	//
	//GYRO
	idx = 3;
	for(int i=0; i<6; i++){
		if(i & 1)	{data[cnt++] = pi16_imu_left[idx++];}
		else		{data[cnt++] = pi16_imu_left[idx] >> 8;}
	}
	//ACCEL
	idx = 0;
	for(int i=0; i<6; i++){
		if(i & 1)	{data[cnt++] = pi16_imu_left[idx++];}
		else		{data[cnt++] = pi16_imu_left[idx] >> 8;}
	}
	//	IMU RIGHT	//
	idx = 3;
	for(int i=0; i<6; i++){
		if(i & 1)	{data[cnt++] = pi16_imu_right[idx++];}
		else		{data[cnt++] = pi16_imu_right[idx] >> 8;}
	}
	//ACCEL
	idx = 0;
	for(int i=0; i<6; i++){
		if(i & 1)	{data[cnt++] = pi16_imu_right[idx++];}
		else		{data[cnt++] = pi16_imu_right[idx] >> 8;}
	}

	//	FSR LEFT	//
	data[cnt++] = u16_fsr_left >> 8;
	data[cnt++] = u16_fsr_left;
	//	FSR RIGHT	//
	data[cnt++] = u16_fsr_right >> 8;
	data[cnt++] = u16_fsr_right;

	//	TEMP Out	//
	temp = f_tempOut * 100;
	data[cnt++] = (uint8_t)f_tempOut;
	data[cnt++] = temp - (int)f_tempOut * 100;

	//	TEMP IN		//
	temp = f_tempIn * 100;
	data[cnt++] = (uint8_t)f_tempIn;
	data[cnt++] = temp - (int)f_tempIn * 100;

	//	TEMP IR		//
	temp = f_tempIR * 100;
	data[cnt++] = (uint8_t)f_tempIR;
	data[cnt++] = temp - (int)f_tempIR * 100;

	//	TOF1	//
	data[cnt++] = u16_tof1 >> 8;
	data[cnt++] = u16_tof1;

	//	TOF2	//
	data[cnt++] = u16_tof2 >> 8;
	data[cnt++] = u16_tof2;

	//	BAT		//
	temp = f_bat * 100;
	data[cnt++] = (uint8_t)f_bat;
	data[cnt++] = temp - (int)f_bat * 100;

	//	IMU EVT	//
	data[cnt++] = u8_imu_left_evt;
	data[cnt++] = u8_imu_right_evt;

	v_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_STAT, data, cnt);

	if(i_toutAct == 0){
		i_toutAct = 1;
		u32_toutRef = u32_Tim_1msGet();
	}
}

/*
 * brief	: ESP <-> STM uart timeout handler
 * date
 * - create	: 25.08.26
 */
void v_ESP_Tout_Handler(){
	if(i_toutAct && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 1000)){
		i_toutAct = 0;
		v_Mode_Set_Error(modeERR_ESP_COMM);
		v_Mode_SetNext(modeERROR);
	}
}



void v_ESP_Send_Error(uint16_t u16_error){
	uint8_t error[4];
	error[0] = u16_error >> 8;
	error[1] = u16_error;	//fixed ->
	v_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_ERR, error, 2);
}







static void v_ESP_Transmit_toRx(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len){
	// CRITICAL: Validate buffer size to prevent stack overflow
	// fmt[64] = STX(1) + LEN(1) + DIR(1) + CMD(1) + DATA(u16_len) + CHK(1) + ETX(1)
	// Maximum safe data length: 64 - 6 = 58 bytes
	if(u16_len > (ESP_TX_TO_RX_BUF_SIZE - 6)){
		return;  // Prevent buffer overflow
	}

	uint8_t fmt[ESP_TX_TO_RX_BUF_SIZE];
	//uint8_t fmt[ESP_FMT_SIZE_MIN + u16_len + 1];
	uint8_t chk = ESP_FMT_CHK_INIT;
	uint16_t cnt=0;

	//STX
	fmt[cnt] = ESP_FMT_STX;
	chk ^= fmt[cnt++];
	//LEN
	fmt[cnt] = ESP_FMT_LEN_MIN + u16_len;
	chk ^= fmt[cnt++];
	//DIR
	fmt[cnt] = u8_dir;
	chk ^= fmt[cnt++];
	//CMD
	fmt[cnt] = u8_cmd;
	chk ^= fmt[cnt++];
	//DATA
	for(uint16_t i=0; i<u16_len; i++){
		fmt[cnt] = pu8_data[i];
		chk ^= fmt[cnt++];
	}
	//CHK
	fmt[cnt++] = chk;
	//ETX
	fmt[cnt++] = ESP_FMT_ETX;	//etx

	for(int i=0; i<cnt; i++){
		v_ESP_Recive(fmt[i]);
	}
}

void v_ESP_CmdTest(){
	if(e_Mode_Get_CurrID() >= modeWAITING){
		static uint32_t timRef=0;
		static uint16_t order = ESP_CMD_CTRL_MODE;
		static uint32_t toggle;
		uint8_t data[12] = {0,};
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 5000)){
			timRef = u32_Tim_1msGet();

			switch(order){
			case ESP_CMD_CTRL_MODE:
				data[0] = toggle;
				v_ESP_Transmit_toRx(ESP_DIR_REQ, ESP_CMD_CTRL_MODE, data, 1);
				break;
			case ESP_CMD_INIT_TEMP_SLEEP:
				data[0] = 35;
				data[1] = 5;
				v_ESP_Transmit_toRx(ESP_DIR_REQ, ESP_CMD_INIT_TEMP_SLEEP, data, 2);
				break;
			case ESP_CMD_INIT_TEMP_WAITING:
				data[0] = 45;
				data[1] = 0;
				v_ESP_Transmit_toRx(ESP_DIR_REQ, ESP_CMD_INIT_TEMP_WAITING, data, 2);
				break;
			case ESP_CMD_INIT_TEMP_FORCEUP:
				data[0] = 60;
				data[1] = 0;
				v_ESP_Transmit_toRx(ESP_DIR_REQ, ESP_CMD_INIT_TEMP_FORCEUP, data, 2);
				break;
			}
#if 0
			if(order < ESP_CMD_INIT_TEMP_FORCEUP){
				order++;
			}
			else{
				order = ESP_CMD_INIT_TEMP_SLEEP;
			}
#endif
			if(toggle < 4){
				toggle++;
			}
			else{
				toggle = 0;
			}
		}
	}
}


