#include <comm_dbg.h>
#include "lib_ringbuf.h"
#include "string.h"
#include "stdio.h"
#include "tim.h"
#include "mode.h"
#include "mlx90640_platform.h"
#include "as6221_platform.h"
//define
#define DBG_RX_ARR_SIZE	(128)
#define DBG_RX_EOL		('\r')

#define DBG_LOG_ENABLED	1


//variable
_RING_VAR_DEF(dbgRx, uint8_t, DBG_RX_ARR_SIZE);

static uint8_t u8_rxArr[DBG_RX_ARR_SIZE + 1];
static uint8_t* pu8_rxArr = u8_rxArr;





static void v_DBG_RxHandler();
static void v_DBG_RxProc(uint8_t u8_cmd, int32_t* pi32_data, uint16_t u16_len);



/*
 * brief	: debug receive data
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void v_DBG_Receive(uint8_t u8_data){
	dbgRx->fn.v_Put(dbgRx, u8_data);
}


/*
 * brief	: debug receive handler
 * date
 * - create	: 25.04.28
 * - modify	: 25.04.28
 * note
 * - end	: \n
 */
bool b_DBG_Ascii_RxHandler(){
	if(dbgRx->u16_cnt){
		uint8_t rx;
		dbgRx->fn.b_Get(dbgRx, &rx);
		if(pu8_rxArr - u8_rxArr < DBG_RX_ARR_SIZE){
			*pu8_rxArr++ = rx;
			printf("%c", rx);
		}

		if(rx == DBG_RX_EOL){
			if(*(pu8_rxArr - 1) == DBG_RX_EOL){
				//null insert
				*(pu8_rxArr - 1) = 0;
				//default
				pu8_rxArr = u8_rxArr;
				printf("\n");
			}
			return true;
		}
	}
	return false;
}

static void v_DBG_Ascii_DataHandler(char* pc8_data);

void v_DBG_Ascii_Handler(){
	if(b_DBG_Ascii_RxHandler()){
		v_DBG_Ascii_DataHandler((char*)pu8_rxArr);
	}
}

static void v_DBG_Ascii_DataHandler(char* pc8_data){
	if(strcmp(pc8_data, "123") == 0){
		printf("abc\n");
	}
	else if(strcmp(pc8_data, "456") == 0){
		printf("def\n");
	}
	else if(strcmp(pc8_data, "789") == 0){
		printf("ghi\n");
	}
	pc8_data = (char*)u8_rxArr;
}

/*
 * brief	: debug handler
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void v_DBG_Handler(){
	v_DBG_RxHandler();
	//v_DBG_Ascii_Handler();
}


void v_DBG_RxTest(){
	if(dbgRx->u16_cnt){
		uint8_t str[128] = {0,};
		dbgRx->fn.b_GetArr(dbgRx, str, dbgRx->u16_cnt);
		printf("%s", str);
	}
}


/*
 * STX	0xFF | 0xFF
 * ETX
 *
 */

typedef enum {
	DBG_CMD_BOOT_CPLT	=0x00,
	DBG_CMD_ENTER_TEST	=0x01,
	DBG_CMD_TEMP_LMT	=0x02,
	DBG_CMD_FAN_SPEED	=0x03,
	DBG_CMD_LMA_INIT	=0x04,
	DBG_CMD_TEMP_HEATER	=0x05,
	DBG_CMD_TEMP_COOL	=0x06,
	DBG_CMD_TEMP_REQ	=0x07,
	DBG_CMD_EXIT_TEST	=0x08,
} e_DBG_CMD_t;

#define DBG_FMT_STX	(uint16_t)0xFFFF	//(2B)
#define DBG_FMT_ETX	(uint16_t)0xFEFE	//(2B)

#define DBG_FMT_SIZE_MIN	6	//STX + ETX + CMD + LEN

#define DBG_FN_CREATE

bool b_DBG_CmdCompare(uint8_t u8_cmd){
	if(u8_cmd >= DBG_CMD_ENTER_TEST && (u8_cmd <= DBG_CMD_EXIT_TEST)){
		return true;
	}
	else{
		return false;
	}
}


static void v_DBG_RxHandler(){
	static uint8_t len;
	//recursive check
	if(dbgRx->u16_cnt < DBG_FMT_SIZE_MIN + len){return;}
	//	format compare	//

	uint16_t out = dbgRx->u16_out;
	uint16_t mask = dbgRx->u16_mask;
	uint8_t* p_arr = dbgRx->p_arr;

	uint8_t cmd;
	int32_t data[64];
	uint16_t dataLen = 0xFFFF;
	uint16_t stx, etx;
	//STX
	stx = p_arr[out] << 8 | p_arr[(out + 1) & mask];
	if(stx != DBG_FMT_STX){
		v_Uart_DBG_DisableIT();
		dbgRx->fn.b_Jmp(dbgRx, 1);
		v_Uart_DBG_EnableIT();
		len = 0;
#if DBG_LOG_ENABLED
		printf("stx failed : %2x %2x\n", p_arr[out], p_arr[(out + 1) & mask]);
#endif
		return;
	}
	out = (out + 2) & mask;
	//CMD
	cmd = p_arr[out];
	if(b_DBG_CmdCompare(cmd) == false){
		v_Uart_DBG_DisableIT();
		dbgRx->fn.b_Jmp(dbgRx, 1);
		v_Uart_DBG_EnableIT();
		len = 0;
#if DBG_LOG_ENABLED
		printf("cmd failed : %2x\n", cmd);
#endif
		return;
	}
	out = (out + 1) & mask;
	//LEN
	len = p_arr[out];
	if(DBG_FMT_SIZE_MIN + len > dbgRx->u16_cnt){
#if DBG_LOG_ENABLED
		printf("len wait.\n");
#endif
		return;
	}
	out = (out + 1) & mask;
	//DATA
	uint16_t pos;

	for(uint16_t i=0; i<len; i++){
		pos = i % 4;
		if(pos){
			data[dataLen] |= p_arr[out] << (3 - pos) * 8;
		}
		else{
			data[++dataLen] = p_arr[out] << (3 - pos) * 8;	//pos : 0 , << 24
		}
		//MSB
		out = (out + 1) & mask;
	}
	//ETX
	etx = p_arr[out] << 8 | (p_arr[(out + 1) & mask]);
	if(etx != DBG_FMT_ETX){
		v_Uart_DBG_DisableIT();
		dbgRx->fn.b_Jmp(dbgRx, 1);
		v_Uart_DBG_EnableIT();
		len = 0;
#if DBG_LOG_ENABLED
		printf("etx failed : %2x %2X\n", p_arr[out], p_arr[(out + 1) & mask]);
#endif
		return;
	}
	//decrease count..
	v_Uart_DBG_DisableIT();
	dbgRx->fn.b_Jmp(dbgRx, DBG_FMT_SIZE_MIN + len);
	v_Uart_DBG_EnableIT();

	//receive process
	v_DBG_RxProc(cmd, data, len);
	len = 0;
}


static void v_DBG_RxProc(uint8_t u8_cmd, int32_t* pi32_data, uint16_t u16_len){
	static int32_t data[16];
	float temp;
	switch(u8_cmd){
	case DBG_CMD_ENTER_TEST:
		v_Mode_DBG_Enter();
		v_DBG_Transmit(DBG_CMD_ENTER_TEST, data, 0);
		break;
	case DBG_CMD_TEMP_LMT:
		//limit
		temp = pi32_data[0] / 10.0;
		v_Mode_Set_DBG_TempMax(temp);
		v_DBG_Transmit(DBG_CMD_TEMP_LMT, data, 0);
		break;
	case DBG_CMD_FAN_SPEED:
		//coolfan
		v_Mode_Set_CoolFan_Now(pi32_data[0]);
		v_DBG_Transmit(DBG_CMD_FAN_SPEED, data, 0);
		break;
	case DBG_CMD_LMA_INIT:
		//forceUp
		temp = pi32_data[0] / 10.0;
		v_Mode_Set_DBG_FoceUp(temp);
		//waiting
		temp = pi32_data[1] / 10.0;
		v_Mode_Set_DBG_Waiting(temp);
		//tout
		v_Mode_Set_DBG_Tout(pi32_data[2]);
		v_Mode_Set_DBG_Act(modeTEST_FORCE_UP);
		//forceUp start
		v_DBG_Transmit(DBG_CMD_LMA_INIT, data, 0);
		break;
	case DBG_CMD_TEMP_HEATER:
		temp = pi32_data[0] / 10.0;
		v_Mode_Set_DBG_FoceUp(temp);
		v_Mode_Set_DBG_Act(modeTEST_FORCE_UP);
		//forceUp start
		v_DBG_Transmit(DBG_CMD_TEMP_HEATER, data, 0);
		break;
	case DBG_CMD_TEMP_COOL:
		temp = pi32_data[0] / 10.0;
		v_Mode_Set_DBG_ForceDown(temp);
		v_Mode_Set_DBG_Act(modeTEST_FORCE_DOWN);
		//forceDown start
		v_DBG_Transmit(DBG_CMD_TEMP_COOL, data, 0);
		break;
	case DBG_CMD_TEMP_REQ:
	{
		int32_t act_temp = f_IR_Temp_Get() * 10;
		int32_t temp_out = f_Temp_Out_Get() * 10;
		data[0] = act_temp;
		data[1] = temp_out;
		v_DBG_Transmit(DBG_CMD_TEMP_REQ, data, 2);
		break;
	}
	case DBG_CMD_EXIT_TEST:
		v_Mode_Set_DBG_Act(modeTEST_WAITING);
		//waiting start
		v_DBG_Transmit(DBG_CMD_EXIT_TEST, data, 0);
		break;
	default:
		break;
	}

}


void v_DBG_Transmit(uint8_t u8_cmd, int32_t* pi32_data, uint16_t u16_len){
	uint8_t buf[64];
	uint16_t cnt = 0;
	uint16_t len = u16_len * 4;
	buf[cnt++] = DBG_FMT_STX >> 8;
	buf[cnt++] = DBG_FMT_STX & 0xFF;
	buf[cnt++] = u8_cmd;
	buf[cnt++] = (uint8_t)len;

	uint16_t pos;
	int data_idx=0;
	for(int i=len-1; i>=0; i--){
		pos = i % 4;
		buf[cnt++] = pi32_data[data_idx] >> (8 * pos);
		if(pos == 0){data_idx++;}
	}
	buf[cnt++] = DBG_FMT_ETX >> 8;
	buf[cnt++] = DBG_FMT_ETX & 0xFF;

	/*
	for(int i=0; i<cnt; i++){
		v_DBG_Receive(buf[i]);
	}
	*/
	//output
	v_Uart_DBG_Out(buf, cnt);
}

void v_DBG_BootComplete(){
	v_DBG_Transmit(DBG_CMD_BOOT_CPLT, NULL, 0);
}


uint8_t u8_dbgRx_in[16];
void v_DBG_CmdTest(){
	static uint32_t timRef;
	static e_DBG_CMD_t cmd;
	int32_t data[4] = {0,};
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 1000)){
		timRef = u32_Tim_1msGet();

		switch(cmd){
		case DBG_CMD_BOOT_CPLT:

			break;
		case DBG_CMD_ENTER_TEST:
			data[0] = 1;
			v_DBG_Transmit(DBG_CMD_ENTER_TEST, data, 1);
			break;
		case DBG_CMD_TEMP_LMT:
			data[0] = 70;
			v_DBG_Transmit(DBG_CMD_TEMP_LMT, data, 1);
			break;
		case DBG_CMD_FAN_SPEED:
			data[0] = 10;
			v_DBG_Transmit(DBG_CMD_FAN_SPEED, data, 1);
			break;
		case DBG_CMD_LMA_INIT:
			data[0] = 60;
			data[1] = 30;
			data[2] = 10;
			v_DBG_Transmit(DBG_CMD_LMA_INIT, data, 3);
			break;
		case DBG_CMD_TEMP_HEATER:
			data[0] = 60;
			v_DBG_Transmit(DBG_CMD_TEMP_HEATER, data, 1);
			break;
		case DBG_CMD_TEMP_COOL:
			data[0] = 25;
			v_DBG_Transmit(DBG_CMD_TEMP_COOL, data, 1);
			break;
		case DBG_CMD_TEMP_REQ:
			v_DBG_Transmit(DBG_CMD_TEMP_REQ, data, 0);
			break;
		case DBG_CMD_EXIT_TEST:
			v_DBG_Transmit(DBG_CMD_EXIT_TEST, data, 0);
			break;
		}

		if(cmd < DBG_CMD_EXIT_TEST){
			cmd++;
		}
		else{
			cmd = DBG_CMD_ENTER_TEST;
		}
		//dbgRx->fn.v_PutArr(dbgRx, );
	}
}







