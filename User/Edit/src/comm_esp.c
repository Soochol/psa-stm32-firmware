#include "comm_esp.h"
#include "lib_ringbuf.h"
#include "stdio.h"
#include "string.h"
#include "mode.h"
#include "tim.h"
#include "math.h"
#include "sam_m10q_platform.h"
#include "minimp3_platform.h"
#include "lib_log.h"
#include "sd.h"
#include "flash_cfg.h"
#include "version.h"


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
	ESP_CMD_INIT_LOG_IDENTITY	=0x23,	//SD logging spec 6.1
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
	ESP_CMD_REQ_LOG_STATUS		=0x43,	//SD logging spec 6.2
	ESP_CMD_REQ_LOG_READ		=0x44,	//SD logging spec 6.4
	ESP_CMD_REQ_LOG_FILES		=0x45,	//SD logging spec 6.3
	ESP_CMD_REQ_DEVICE_VERSION	=0x46,	//SPEC_PROPOSAL_reqDeviceVersion
	//CONTROL
	ESP_CMD_CTRL_RST			=0x50,
	ESP_CMD_CTRL_MODE			=0x51,	//add
	ESP_CMD_CTRL_SPK_ON			=0x52,
	ESP_CMD_CTRL_BLOWFAN_ON		=0x53,
	ESP_CMD_CTRL_SPK_PLAY		=0x54,
	ESP_CMD_CTRL_COOLFAN_ON		=0x55,
	ESP_CMD_CTRL_LOG_EN			=0x56,	//SD logging spec 6.6
	ESP_CMD_CTRL_LOG_DEL		=0x57,	//SD logging spec 6.8
	//STATUS
	ESP_CMD_STAT=0x70,
	ESP_CMD_STAT_LOG_CHUNK=0x71,	//SD logging spec 6.5
	//EVENT
	ESP_CMD_EVT_INIT_START=0x80,
    ESP_CMD_EVT_INIT_RESULT=0x81,
    ESP_CMD_EVT_MODE=0x82,
    ESP_CMD_EVT_WARN=0x83,
	ESP_CMD_EVT_LOG_ERR=0x84,	//SD logging spec 6.7
	//ERROR
	ESP_CMD_ERR=0x90,
} e_ESP_CMD_t;


// Category bounds of the ESP32 command map. The parser accepts a whole category
// rather than stopping at the last command implemented here, so a newly assigned
// code reaches its handler instead of being discarded before the checksum check
// (which also drops the receiver into byte-by-byte resync).
//
// Bounds are deliberately NOT derived from the enum above: using the last
// defined command as the upper bound is what made the parser reject every code
// the SD logging extension assigns (0x23 / 0x43 / 0x44 / 0x45 / 0x56 / 0x71 /
// 0x84). Codes inside a category with no case in their handler are ACKed
// without action.
#define ESP_CMD_INIT_MIN	(0x10)
#define ESP_CMD_INIT_MAX	(0x29)
#define ESP_CMD_REQ_MIN		(0x30)
#define ESP_CMD_REQ_MAX		(0x49)
#define ESP_CMD_CTRL_MIN	(0x50)
#define ESP_CMD_CTRL_MAX	(0x69)
#define ESP_CMD_STAT_MIN	(0x70)
#define ESP_CMD_STAT_MAX	(0x79)
#define ESP_CMD_EVT_MIN		(0x80)
#define ESP_CMD_EVT_MAX		(0x89)
#define ESP_CMD_ERR_MIN		(0x90)
#define ESP_CMD_ERR_MAX		(0x99)






#define ESP_RX_ARR_SIZE		(128)

// One statLogChunk DATA frame, and the room kept free ahead of it so the live
// STAT frame (70 B) never has to queue behind a burst of them.
//
// The reserve has to cover everything that can be sent between two runs of the
// pump, not the STAT alone. It was 128, which is what STAT needs plus a little,
// and that is not enough: a reqLogFiles reply is 6 entries of 14 B plus 2, or
// 92 B on the wire, and ESP32 polls for it while a backfill is running. 92 + 70
// overruns 128, the STAT is refused, and the sample goes to the card with
// SD_FLAG_TX_OK clear -- 4.5 % of them during backfill, measured by ESP32 off
// the card in their report #23.
//
// Sized from the frame buffer instead of from one command: no frame can exceed
// ESP_TX_FMT_BUF_SIZE, so a largest frame plus a STAT plus slack for the small
// ACKs that follow a request is the bound. The pump then needs 343 B free of
// the 2 KB ring, still ~19 chunks of headroom.
#define ESP_BF_FRAME_SIZE	(6 + 81)
#define ESP_BF_TX_RESERVE	(256)


//function
static bool b_ESP_Transmit(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len);
static void v_ESP_RxHandler();
static void v_ESP_RxProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_RxAck(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);

static void v_ESP_InitProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_ReqProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_CtrlProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);
static void v_ESP_StatProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len);

//vairlabe
_RING_VAR_DEF(espRx, uint8_t, ESP_RX_ARR_SIZE);
static uint32_t u32_toutRef;
static int i_toutAct;





void v_ESP_Handler(){
	v_ESP_RxHandler();
	v_ESP_LogError_Handler();
	v_ESP_Backfill_Handler();
}

/*
 * brief	: stream statLogChunk(0x71) frames while a backfill is running
 * date
 * - create	: 26.08.11
 * note
 * - Live STAT keeps priority (spec section 9.3) by reservation rather than by
 *   preemption: chunks stop once the TX ring drops below ESP_BF_TX_RESERVE, so
 *   the sensing tick always finds room for its 70 B frame. Nothing has to be
 *   cut mid-stream, which is what "interrupt at a frame boundary" needs anyway.
 * - No ACK per data chunk (spec section 9.2). A round trip per record would cost
 *   more than the record; gaps are found by seq continuity and re-requested.
 */
void v_ESP_Backfill_Handler(){
	uint8_t u8_chunk[1 + 80];
	uint8_t u8_result = 0;

	if(!b_SD_Log_Backfill_Active()) return;

	while(u16_Uart_ESP_TxFree() >= (ESP_BF_FRAME_SIZE + ESP_BF_TX_RESERVE)){
		u8_chunk[0] = 0x01;						// DATA chunk
		if(!b_SD_Log_Backfill_Next(&u8_chunk[1], &u8_result)){
			uint32_t u32_last = u32_SD_Log_Backfill_LastSent();
			uint8_t  u8_end[6];
			u8_end[0] = 0x00;					// END chunk
			u8_end[1] = u8_result;
			u8_end[2] = (uint8_t)(u32_last >> 24);
			u8_end[3] = (uint8_t)(u32_last >> 16);
			u8_end[4] = (uint8_t)(u32_last >> 8);
			u8_end[5] = (uint8_t)(u32_last);
			b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_STAT_LOG_CHUNK, u8_end, 6);
			LOG_INFO("COMM_ESP", "backfill end result=%u lastSeqSent=%u",
					u8_result, (unsigned)u32_last);
			return;
		}
		if(!b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_STAT_LOG_CHUNK, u8_chunk, 1 + 80)){
			// Should not happen given the reserve, but a dropped chunk must not
			// be silently skipped: end the stream so the PC re-requests from
			// lastSeqSent + 1 rather than seeing a hole it cannot explain.
			v_SD_Log_Backfill_Abort(4);
			return;
		}
	}
}

/*
 * brief	: forward a pending SD logging fault as evtLogError(0x84)
 * date
 * - create	: 26.08.11
 * note
 * - Pushed as soon as it happens rather than waiting for the next reqLogStatus
 *   poll, which is a minute away (spec section 6.7).
 */
void v_ESP_LogError_Handler(){
	uint8_t  u8_reason;
	uint16_t u16_detail;

	if(!b_SD_Log_Get_Error(&u8_reason, &u16_detail)) return;

	uint8_t data[3];
	data[0] = u8_reason;
	data[1] = (uint8_t)(u16_detail >> 8);
	data[2] = (uint8_t)(u16_detail);
	b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_LOG_ERR, data, 3);
	LOG_WARN("COMM_ESP", "evtLogError reason=%u detail=%u", u8_reason, u16_detail);
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
static bool b_ESP_Transmit(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len){
	// CRITICAL: Validate buffer size to prevent stack overflow
	// fmt = STX + LEN + DIR + CMD + DATA(u16_len) + CHK + ETX, so the bound on
	// DATA is the buffer less the ESP_FMT_SIZE_MIN framing bytes.
	// Written against the macros rather than as numbers: this comment was cloned
	// from v_ESP_Transmit_toRx, whose buffer really is 64, and kept saying 58
	// after this one grew to 96 -- which read as though reqLogFiles' 86 B were
	// already over the limit.
	if(u16_len > (ESP_TX_FMT_BUF_SIZE - ESP_FMT_SIZE_MIN)){
		return false;  // Prevent buffer overflow
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

	return b_Uart_ESP_Out(fmt, cnt);
}

/*
 * brief	: command compare
 * date
 * - create	: 25.04.28
 * - modify	: 25.04.29
 */
bool b_ESP_CmdCompare(uint8_t u8_cmd){
	if((u8_cmd >= ESP_CMD_INIT_MIN && u8_cmd <= ESP_CMD_INIT_MAX)\
	||(u8_cmd >= ESP_CMD_REQ_MIN && u8_cmd <= ESP_CMD_REQ_MAX)\
	||(u8_cmd >= ESP_CMD_CTRL_MIN && u8_cmd <= ESP_CMD_CTRL_MAX)\
	||(u8_cmd >= ESP_CMD_STAT_MIN && u8_cmd <= ESP_CMD_STAT_MAX)\
	||(u8_cmd >= ESP_CMD_EVT_MIN && u8_cmd <= ESP_CMD_EVT_MAX)\
	||(u8_cmd >= ESP_CMD_ERR_MIN && u8_cmd <= ESP_CMD_ERR_MAX)){
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
	uint8_t data[ESP_RX_DATA_BUF_SIZE];

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
	// CRITICAL FIX: Validate data_len to prevent buffer overflow. Named after the
	// buffer it guards so the two cannot drift; equality fits exactly.
	if(data_len > ESP_RX_DATA_BUF_SIZE){
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
	if(u8_cmd >= ESP_CMD_CTRL_MIN && u8_cmd <= ESP_CMD_CTRL_MAX){
		//CTRL
		v_ESP_CtrlProc(u8_cmd, pu8_data, u8_len);
	}
	else if(u8_cmd >= ESP_CMD_INIT_MIN && u8_cmd <= ESP_CMD_INIT_MAX){
		//INIT
		v_ESP_InitProc(u8_cmd, pu8_data, u8_len);
	}
	else if(u8_cmd >= ESP_CMD_REQ_MIN && u8_cmd <= ESP_CMD_REQ_MAX){
		//REQ
		v_ESP_ReqProc(u8_cmd, pu8_data, u8_len);
	}
}

/*
 * note
 * - ACKs for STM->ESP messages we do not track yet (evtWarn, evtLogError...)
 *   fall through here on purpose. The frame is still consumed correctly because
 *   b_ESP_CmdCompare accepts the whole category; only the reaction is missing.
 *   Add a case together with the command that needs its ACK observed.
 */
static void v_ESP_RxAck(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	if(u8_cmd == ESP_CMD_STAT){
		v_ESP_StatProc(u8_cmd, pu8_data, u8_len);
	}
}

// LOW: Removed unused variable 'in_cmd'
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
	// LOW: Removed unused assignment to 'in_cmd'

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
	case ESP_CMD_INIT_LOG_IDENTITY:
		// Stored non-volatile so the next boot can name its directory before the
		// ESP32 says anything. Any file already open keeps the name it was created
		// with — spec 6.1 forbids rewriting a header after the fact — so the new
		// id first appears at the next rotation.
		if(u8_len >= FLASH_CFG_DEVID_LEN){
			v_Flash_Cfg_Set_DeviceId(pu8_data);
			LOG_INFO("COMM_ESP", "deviceId %02X%02X%02X%02X%02X%02X",
					pu8_data[0], pu8_data[1], pu8_data[2],
					pu8_data[3], pu8_data[4], pu8_data[5]);
		}
		break;
	}
	//ack
	b_ESP_Transmit(ESP_DIR_ACK, u8_cmd, NULL, 0);
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
static void v_ESP_ReqProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	// 0x45 answers with up to 2 + 14*6 = 86 B, past the 32 the older commands need.
	static uint8_t data[90];
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
	case ESP_CMD_REQ_LOG_FILES:
	{
		// startIndex paginates; the wire limit of 90 B caps a page at 6 entries.
		uint16_t u16_total = u16_SD_Log_Idx_Count();
		uint8_t  u8_start  = (u8_len >= 1) ? pu8_data[0] : 0;
		uint8_t  u8_ret    = 0;

		data[len++] = (u16_total > 255) ? 255 : (uint8_t)u16_total;
		len++;								// returnedCount, filled in below
		for(uint8_t i = 0; i < 6; i++){
			uint32_t u32_boot, u32_first, u32_last;
			uint16_t u16_idx;
			if(!b_SD_Log_Idx_Get((uint16_t)(u8_start + i), &u32_boot, &u32_first,
					&u32_last, &u16_idx)) break;
			data[len++] = u32_boot >> 24;  data[len++] = u32_boot >> 16;
			data[len++] = u32_boot >> 8;   data[len++] = u32_boot;
			data[len++] = u32_first >> 24; data[len++] = u32_first >> 16;
			data[len++] = u32_first >> 8;  data[len++] = u32_first;
			data[len++] = u32_last >> 24;  data[len++] = u32_last >> 16;
			data[len++] = u32_last >> 8;   data[len++] = u32_last;
			data[len++] = u16_idx >> 8;    data[len++] = u16_idx;
			u8_ret++;
		}
		data[1] = u8_ret;
		break;
	}
	case ESP_CMD_REQ_LOG_READ:
	{
		// ACK first, then the chunk stream (spec 6.4). A refusal is reported as an
		// END chunk rather than in the ACK, so the PC has one place to look.
		if(u8_len < 10) break;
		uint32_t u32_boot  = ((uint32_t)pu8_data[0] << 24) | ((uint32_t)pu8_data[1] << 16)
		                   | ((uint32_t)pu8_data[2] << 8)  |  (uint32_t)pu8_data[3];
		uint32_t u32_start = ((uint32_t)pu8_data[4] << 24) | ((uint32_t)pu8_data[5] << 16)
		                   | ((uint32_t)pu8_data[6] << 8)  |  (uint32_t)pu8_data[7];
		uint16_t u16_cnt   = (uint16_t)(((uint16_t)pu8_data[8] << 8) | pu8_data[9]);

		uint8_t u8_res = u8_SD_Log_Backfill_Start(u32_boot, u32_start, u16_cnt);
		LOG_INFO("COMM_ESP", "reqLogRead boot=%u seq=%u cnt=%u -> %u",
				(unsigned)u32_boot, (unsigned)u32_start, u16_cnt, u8_res);
		if(u8_res != 0xFF){
			b_ESP_Transmit(ESP_DIR_ACK, u8_cmd, NULL, 0);
			uint8_t u8_end[6] = {0x00, u8_res, 0, 0, 0, 0};
			uint32_t u32_last = u32_SD_Log_Backfill_LastSent();
			u8_end[2] = (uint8_t)(u32_last >> 24); u8_end[3] = (uint8_t)(u32_last >> 16);
			u8_end[4] = (uint8_t)(u32_last >> 8);  u8_end[5] = (uint8_t)(u32_last);
			b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_STAT_LOG_CHUNK, u8_end, 6);
			return;						// ACK already sent
		}
		break;
	}
	case ESP_CMD_REQ_LOG_STATUS:
	{
		// 25 B, spec 6.2. currentTickMs is read here rather than carried from
		// elsewhere: it is the anchor the merge tool pairs with absolute time, and
		// pairing is only tight if it is sampled as the reply is built.
		uint32_t u32_bootId  = u32_Flash_Cfg_Get_BootId();
		uint32_t u32_lastSeq = u32_SD_Log_Get_FlushedSeq();
		uint32_t u32_tick    = u32_Tim_1msGet();
		uint32_t u32_fileIdx = u32_SD_Log_Get_FileIndex();
		uint16_t u16_free    = u16_SD_Log_Get_FreeMB();
		uint16_t u16_wrErr   = u16_SD_Log_Get_WriteErrCnt();

		data[len++] = u8_SD_Log_Get_State();
		data[len++] = u32_bootId >> 24;   data[len++] = u32_bootId >> 16;
		data[len++] = u32_bootId >> 8;    data[len++] = u32_bootId;
		data[len++] = u32_lastSeq >> 24;  data[len++] = u32_lastSeq >> 16;
		data[len++] = u32_lastSeq >> 8;   data[len++] = u32_lastSeq;
		data[len++] = u32_tick >> 24;     data[len++] = u32_tick >> 16;
		data[len++] = u32_tick >> 8;      data[len++] = u32_tick;
		data[len++] = u16_free >> 8;      data[len++] = u16_free;
		// fileIndex is uint32 in the file header but uint16 on the wire; clamp
		// rather than wrap, so a saturated value reads as "at least this many".
		if(u32_fileIdx > 0xFFFFU) u32_fileIdx = 0xFFFFU;
		data[len++] = u32_fileIdx >> 8;   data[len++] = u32_fileIdx;
		data[len++] = u16_wrErr >> 8;     data[len++] = u16_wrErr;
		data[len++] = 1;                  // formatVersion
		data[len++] = u8_Mode_Get_ProtoMode();
		// filesOnCard, appended at 23 B (spec 6.2). It is the count and not a
		// "full" flag so the reader can act before the cap rather than after:
		// past it, files are already unreachable and reqLogFiles reports the cap
		// either way. evtLogError(reason 6) says it happened; this says how it
		// stands, which is what survives an ESP32 restart.
		uint16_t u16_files = u16_SD_Log_Files_On_Card();
		data[len++] = u16_files >> 8;     data[len++] = u16_files;
		// indexCapacity, appended at 25 B (spec 6.2). filesOnCard only means
		// anything against the cap, and the reader used to keep its own copy of
		// it: the reduced build for the reason 6 test left the two disagreeing
		// until both were edited by hand. The cap is a start-up blocking budget
		// rather than a card limit, so it moves whenever that budget is retuned
		// -- and the two sides are not reflashed together, which is why it has
		// to travel with the count instead of being agreed once.
		uint16_t u16_cap = u16_SD_Log_Idx_Capacity();
		data[len++] = u16_cap >> 8;       data[len++] = u16_cap;
		break;
	}
	case ESP_CMD_REQ_DEVICE_VERSION:
		// 16 B ASCII; strncpy zero-pads the remainder, which is exactly the
		// wire format. Length is capped at build time in version.h.
		strncpy((char*)&data[len], STM_FW_VERSION, 16);
		len += 16;
		break;
	}
	//ack
	b_ESP_Transmit(ESP_DIR_ACK, u8_cmd, data, len);
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
		LOG_INFO("COMM_ESP", "ESP32 reset command (ESP_CMD_CTRL_RST=0x50), prev_mode=%d", e_Mode_Get_CurrID());
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
	case ESP_CMD_CTRL_SPK_PLAY:
		if(pu8_data[0] >= 1 && pu8_data[0] <= 33){
			i_MP3_Begin(pu8_data[0]);
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
	case ESP_CMD_CTRL_LOG_EN:
		// 0 = stop (flush + close, card safe to pull), 1 = start.
		// Length checked because a malformed frame reaching here would otherwise
		// read past the data the sender actually sent.
		if(u8_len >= 1){
			v_SD_Log_SetEnabled(pu8_data[0] != 0);
		}
		break;
	case ESP_CMD_CTRL_LOG_DEL:
	{
		// bootId(4) + fileIndex(2), the same pair reqLogFiles(0x45) reports, so
		// the request names a file the PC has already been shown.
		//
		// Whether the file may go is the PC's call: only it knows the records
		// arrived, held their CRC and merged. Nothing about that is visible from
		// here, so the driver checks only what would break if the file vanished
		// this instant, and the reply says which of those it was.
		uint8_t u8_res = 4;
		if(u8_len >= 6){
			uint32_t u32_boot = ((uint32_t)pu8_data[0] << 24) | ((uint32_t)pu8_data[1] << 16)
			                  | ((uint32_t)pu8_data[2] << 8)  |  (uint32_t)pu8_data[3];
			uint16_t u16_idx  = (uint16_t)(((uint16_t)pu8_data[4] << 8) | pu8_data[5]);
			u8_res = u8_SD_Log_Delete(u32_boot, u16_idx);
			LOG_INFO("COMM_ESP", "ctrlLogDelete boot=%u file=%u -> %u",
					(unsigned)u32_boot, (unsigned)u16_idx, u8_res);
		}
		b_ESP_Transmit(ESP_DIR_ACK, u8_cmd, &u8_res, 1);
		return;						// ACK already sent, with the result in it
	}
	}
	//ack
	b_ESP_Transmit(ESP_DIR_ACK, u8_cmd, NULL, 0);
}


static void v_ESP_StatProc(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){
	if(u8_cmd == ESP_CMD_STAT){
		i_toutAct = 0;
	}
}

void v_ESP_Send_InitStart(){
	b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_INIT_START, NULL, 0);
}

void v_ESP_Send_InitEnd(){
	b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_INIT_RESULT, NULL, 0);
}


/*
 * brief	: event - mode change (stm -> esp)
 * date
 * - create	: 25.05.07
 * - modify	: -
 */
void v_ESP_Send_EvtModeChange(uint8_t u8_mode){
	b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_MODE, &u8_mode, 1);
}

void v_ESP_Send_Sensing(int16_t* pi16_imu_left, int16_t* pi16_imu_right,\
						uint16_t u16_fsr_left, uint16_t u16_fsr_right,\
						float f_tempOut, float f_tempIn, float f_tempIR,\
						uint16_t u16_tof1, uint16_t u16_tof2, float f_bat,\
						uint8_t u8_imu_left_evt, uint8_t u8_imu_right_evt,\
						_x_XYZ_t angle_left, _x_XYZ_t angle_right){
	uint8_t data[ESP_SENSING_DATA_BUF_SIZE] = {0,};
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

	// ===== GPS Data (10 bytes) =====
	_x_GPS_PVT_t* px_gps = px_GPS_GetPVT();

	if (px_gps != NULL && px_gps->fixType != GPS_FIX_NONE) {
		// 1. Convert int32 → float (1e-7 → degrees)
		float latitude  = (float)px_gps->lat / 1e7f;
		float longitude = (float)px_gps->lon / 1e7f;

		// 2. Big-Endian packing for ESP32
		uint32_t lat_bits, lon_bits;
		memcpy(&lat_bits, &latitude, 4);
		memcpy(&lon_bits, &longitude, 4);

		// Latitude (4 bytes, Big-Endian)
		data[cnt++] = (lat_bits >> 24) & 0xFF;  // MSB
		data[cnt++] = (lat_bits >> 16) & 0xFF;
		data[cnt++] = (lat_bits >> 8)  & 0xFF;
		data[cnt++] = lat_bits & 0xFF;          // LSB

		// Longitude (4 bytes, Big-Endian)
		data[cnt++] = (lon_bits >> 24) & 0xFF;  // MSB
		data[cnt++] = (lon_bits >> 16) & 0xFF;
		data[cnt++] = (lon_bits >> 8)  & 0xFF;
		data[cnt++] = lon_bits & 0xFF;          // LSB

		// Satellites (1 byte)
		data[cnt++] = px_gps->numSV;

		// Fix Type (1 byte)
		data[cnt++] = px_gps->fixType;
	} else {
		// GPS unavailable or no fix - send zeros
		for (int i = 0; i < 10; i++) {
			data[cnt++] = 0;
		}
	}

	// ===== ANGLES (12 bytes, big-endian, int16 scale ×100) =====
	// Order: L.x, L.y, L.z, R.x, R.y, R.z
	_x_XYZ_t angles[2] = { angle_left, angle_right };
	for(int imu = 0; imu < 2; imu++){
		float axis[3] = { angles[imu].x, angles[imu].y, angles[imu].z };
		for(int i = 0; i < 3; i++){
			float v = axis[i];
			if(isnan(v) || isinf(v)) v = 0.0f;  // NaN guard
			int16_t scaled = (int16_t)(v * 100.0f);
			data[cnt++] = (uint8_t)(scaled >> 8);   // MSB (big-endian)
			data[cnt++] = (uint8_t)(scaled & 0xFF); // LSB
		}
	}

	bool b_txOk = b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_STAT, data, cnt);

	// SD sensor log. deviceMode and errorMask are read here rather than inside the
	// driver so sd.c stays a driver — it has no business reaching up into mode.
	v_SD_Log_Write(data, cnt, u8_Mode_Get_ProtoMode(),
			(uint16_t)e_Mode_Get_Error(), b_txOk);

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
		LOG_WARN("COMM_ESP", "ESP STAT response timeout - retrying");
	}
}



void v_ESP_Send_Error(uint16_t u16_error){
	uint8_t error[4];
	error[0] = u16_error >> 8;
	error[1] = u16_error;	//fixed ->
	b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_ERR, error, 2);
}

void v_ESP_Send_Warning(uint8_t u8_warn_type){
	b_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_EVT_WARN, &u8_warn_type, 1);
}







static void v_ESP_Transmit_toRx(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len){
	// CRITICAL: Validate buffer size to prevent stack overflow
	// fmt = STX + LEN + DIR + CMD + DATA(u16_len) + CHK + ETX, so the bound on
	// DATA is the buffer less the ESP_FMT_SIZE_MIN framing bytes.
	if(u16_len > (ESP_TX_TO_RX_BUF_SIZE - ESP_FMT_SIZE_MIN)){
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

