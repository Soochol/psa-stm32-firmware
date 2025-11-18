#include "main.h"
#include "as6221_platform.h"
#include "tim.h"
#include "string.h"
#include "i2c.h"
#include "stdio.h"
#include "mode.h"
#include "lib_log.h"


static _AS6221_DRV_t x_drv;
static _AS6221_DRV_t* px_drv = &x_drv;


#define TEMP_LSB_UNIT	(0.0078125)
#define AS6221_ADDR	(0x49 << 1)
#define TEMP_ADDR_OUTDOOR	(0x49 << 1)
#define TEMP_ADDR_INDOOR	(0x48 << 1)



#define AS6221_WR_ARR_SIZE	16
#define AS6221_RD_ARR_SIZE	16

static uint8_t u8_wr[AS6221_WR_ARR_SIZE + 1];



//#define AS6221_I2C_POLL

extern I2C_HandleTypeDef hi2c1;
static I2C_HandleTypeDef* p_i2c = &hi2c1;



uint16_t u16_tempOut, u16_tempIn;
float tempOutDoor, tempInDoor;

static uint32_t u32_toutRef_In, u32_toutRef_Out;
static e_COMM_STAT_t e_temp_in_evt, e_temp_out_evt;

/*
 * brief	: write done handler
 * date
 * - create	: 25.05.21
 * - modify	: -
 * note
 * - place in i2c write done handler
 */
void v_AS6221_Write_DoneHandler(uint8_t u8_addr){
	if(u8_addr == ADDR_TEMP_INDOOR){
		e_temp_in_evt = COMM_STAT_DONE;
	}
	else{
		e_temp_out_evt = COMM_STAT_DONE;
	}
}



/*
 * brief	: read done handler
 * date
 * - create	: 25.05.21
 * - modify	: -
 * note
 * - place in i2c read done handler
 */
void v_AS6221_Read_DoneHandler(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_len){
	if(u8_addr == ADDR_TEMP_INDOOR){
		u16_tempIn = pu8_arr[0] << 8;
		u16_tempIn |= pu8_arr[1];
		tempInDoor = u16_tempIn * TEMP_LSB_UNIT;
		e_temp_in_evt = COMM_STAT_DONE;
	}
	else{
		u16_tempOut = pu8_arr[0] << 8;
		u16_tempOut |= pu8_arr[1];
		tempOutDoor = u16_tempOut * TEMP_LSB_UNIT;
		e_temp_out_evt = COMM_STAT_DONE;
	}
}




int i_Temp_InOut_Write(uint8_t u8_addr, uint16_t u16_memAddr, uint8_t* pu8, uint16_t u16_cnt){
	if(u8_addr == ADDR_TEMP_INDOOR){
		u32_toutRef_In = u32_Tim_1msGet();
		e_temp_in_evt = COMM_STAT_BUSY;
	}
	else{
		u32_toutRef_Out = u32_Tim_1msGet();
		e_temp_out_evt = COMM_STAT_BUSY;
	}
	return i_I2C1_Write(u8_addr, u16_memAddr, pu8, u16_cnt);
}


int i_Temp_InOut_Read(uint8_t u8_addr, uint16_t u16_memAddr, uint16_t u16_cnt){
	if(u8_addr == ADDR_TEMP_INDOOR){
		u32_toutRef_In = u32_Tim_1msGet();
		e_temp_in_evt = COMM_STAT_BUSY;
	}
	else{
		u32_toutRef_Out = u32_Tim_1msGet();
		e_temp_out_evt = COMM_STAT_BUSY;
	}
	return i_I2C1_Read(u8_addr, u16_memAddr, u16_cnt);
}

static e_COMM_STAT_t e_temp_inout_config;

void v_Temp_InOut_Tout_Handler(){
	if((e_temp_in_evt == COMM_STAT_BUSY) && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef_In, 2000)){
		// MEDIUM: Abort DMA transaction to prevent I2C bus lockup
		LOG_ERROR("AS6221", "TEMP_INDOOR I2C1 timeout (addr=0x%02X)", ADDR_TEMP_INDOOR);
		LOG_ERROR("AS6221", "  ISR=0x%08lX (BUSY=%d, STOPF=%d)",
		       p_i2c->Instance->ISR,
		       (p_i2c->Instance->ISR & 0x8000) ? 1 : 0,  // BUSY bit
		       (p_i2c->Instance->ISR & 0x0020) ? 1 : 0); // STOPF bit
		LOG_ERROR("AS6221", "  ErrorCode=0x%08lX", p_i2c->ErrorCode);
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_BERR)    LOG_ERROR("AS6221", "    - Bus Error");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_ARLO)    LOG_ERROR("AS6221", "    - Arbitration Lost");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_AF)      LOG_ERROR("AS6221", "    - NACK (device not responding)");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_OVR)     LOG_ERROR("AS6221", "    - Overrun");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) LOG_ERROR("AS6221", "    - HAL Timeout");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_DMA)     LOG_ERROR("AS6221", "    - DMA Error");

		HAL_I2C_Master_Abort_IT(p_i2c, ADDR_TEMP_INDOOR);
		e_temp_in_evt = COMM_STAT_READY;
		e_temp_inout_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_TEMP_IN);  // FIX 1.00.35: Was modeERR_TEMP_OUT (copy-paste bug)
		v_Mode_SetNext(modeERROR);
	}

	if((e_temp_out_evt == COMM_STAT_BUSY) && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef_Out, 2000)){
		// MEDIUM: Abort DMA transaction to prevent I2C bus lockup
		LOG_ERROR("AS6221", "TEMP_OUTDOOR I2C1 timeout (addr=0x%02X)", ADDR_TEMP_OUTDOOR);
		LOG_ERROR("AS6221", "  ISR=0x%08lX (BUSY=%d, STOPF=%d)",
		       p_i2c->Instance->ISR,
		       (p_i2c->Instance->ISR & 0x8000) ? 1 : 0,  // BUSY bit
		       (p_i2c->Instance->ISR & 0x0020) ? 1 : 0); // STOPF bit
		LOG_ERROR("AS6221", "  ErrorCode=0x%08lX", p_i2c->ErrorCode);
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_BERR)    LOG_ERROR("AS6221", "    - Bus Error");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_ARLO)    LOG_ERROR("AS6221", "    - Arbitration Lost");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_AF)      LOG_ERROR("AS6221", "    - NACK (device not responding)");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_OVR)     LOG_ERROR("AS6221", "    - Overrun");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) LOG_ERROR("AS6221", "    - HAL Timeout");
		if(p_i2c->ErrorCode & HAL_I2C_ERROR_DMA)     LOG_ERROR("AS6221", "    - DMA Error");

		HAL_I2C_Master_Abort_IT(p_i2c, ADDR_TEMP_OUTDOOR);
		e_temp_out_evt = COMM_STAT_READY;
		e_temp_inout_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_TEMP_OUT);
		v_Mode_SetNext(modeERROR);
	}
}

/*
 * 15	: SS		0
 * 14	: Reserved	1
 * 13	: Reserved	0
 * 12	: CF[1]		0
 * 11	: CF[0]		0
 * 10	: POL		0
 * 9	: IM		0
 * 8	: SM		0
 * 7	: CR[1]		1
 * 6	: CR[0]		0
 * 5	: AL		1
 * 4	: Reserved	0
 * 3	: Reserved	0
 * 2	: Reserved	0
 * 1	: Reserved	0
 * 0	: Reserved	0
 */
#define AS6221_CONFIG_POR	0x40A0

void v_Temp_InOut_Deinit(){
	e_temp_inout_config = COMM_STAT_READY;
	e_temp_in_evt = COMM_STAT_READY;
	e_temp_out_evt = COMM_STAT_READY;
}

e_COMM_STAT_t e_Temp_InOut_Ready(){
	static int config;
	uint8_t wr[4];

	if(e_temp_inout_config == COMM_STAT_READY){
		wr[0] = AS6221_CONFIG_POR >> 8;
		wr[1] = AS6221_CONFIG_POR & 0x00FF;
		if(!(config & 0x01) && e_temp_in_evt != COMM_STAT_BUSY){
			if(i_Temp_InOut_Write(ADDR_TEMP_INDOOR, AS6221_REG_CONFIG, wr, 2) == COMM_STAT_OK){
				config |= 0x01;
			}
		}
		if(!(config & 0x02) && e_temp_out_evt != COMM_STAT_BUSY){
			if(i_Temp_InOut_Write(ADDR_TEMP_OUTDOOR, AS6221_REG_CONFIG, wr, 2) == COMM_STAT_OK){
				config |= 0x02;
			}
		}
		if(config == 0x03){
			e_temp_inout_config = COMM_STAT_DONE;
		}
	}
	return e_temp_inout_config;
}




void v_Temp_InOut_Handler(){
	static uint32_t timRef;
	static uint32_t timItv;
	static uint16_t mask;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
		if(mask == 0x00){
			timRef = u32_Tim_1msGet();
			timItv = 0;
		}
		if(!(mask & 0x01) && e_temp_in_evt != COMM_STAT_BUSY){
			if(i_Temp_InOut_Read(ADDR_TEMP_INDOOR, AS6221_REG_TVAL, 2) == COMM_STAT_OK){
				mask |= 0x01;
			}
		}
		if(!(mask & 0x02) && e_temp_out_evt != COMM_STAT_BUSY){
			if(i_Temp_InOut_Read(ADDR_TEMP_OUTDOOR, AS6221_REG_TVAL, 2) == COMM_STAT_OK){
				mask |= 0x02;
			}
		}
		if(mask == 0x03){
			mask = 0x00;
			timItv = 100;
		}
	}
}



float f_Temp_In_Get(){
	return tempInDoor;
}

float f_Temp_Out_Get(){
	return tempOutDoor;
}


uint16_t i2c_tx_failed;
uint16_t i2c_rx_failed;
#define AS6221_DMA	0

#define AS6221_MEM_FN	0


void v_AS6221_Test(){
	static uint32_t timRef;
	static uint16_t toggle;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 500)){
		timRef = u32_Tim_1msGet();
		toggle++;
#if !AS6221_DMA
		HAL_StatusTypeDef ret;

#if AS6221_MEM_FN
		ret = HAL_I2C_Mem_Read(p_i2c, TEMP_ADDR_OUTDOOR, AS6221_REG_TVAL, I2C_MEMADD_SIZE_8BIT, px_drv->rd.pu8_arr, 2, 1000);
		if(ret != HAL_OK){
			i2c_tx_failed++;
		}
		else{
			i2c_tx_failed = 0;
			u16_tempOut = px_drv->rd.pu8_arr[0] << 8;
			u16_tempOut |= px_drv->rd.pu8_arr[1];
			tempOutDoor = u16_tempOut * TEMP_LSB_UNIT;
		}

#else
		u8_wr[0] = AS6221_REG_TVAL;
		ret = HAL_I2C_Master_Transmit(p_i2c, TEMP_ADDR_OUTDOOR, u8_wr, 1, 250);
		if(ret != HAL_OK){
			i2c_tx_failed++;
		}
		else{
			i2c_tx_failed = 0;
		}

		ret = HAL_I2C_Master_Receive(p_i2c, TEMP_ADDR_OUTDOOR, px_drv->rd.pu8_arr, 2, 250);
		if(ret != HAL_OK){
			i2c_rx_failed++;
		}
		else{
			i2c_rx_failed = 0;
			u16_tempOut = px_drv->rd.pu8_arr[0] << 8;
			u16_tempOut |= px_drv->rd.pu8_arr[1];
			tempOutDoor = u16_tempOut * TEMP_LSB_UNIT;
		}
#endif

#if AS6221_MEM_FN
		ret = HAL_I2C_Mem_Read(p_i2c, TEMP_ADDR_INDOOR, AS6221_REG_TVAL, I2C_MEMADD_SIZE_8BIT, px_drv->rd.pu8_arr, 2, 1000);
		if(ret != HAL_OK){
			i2c_tx_failed++;
		}
		else{
			i2c_tx_failed = 0;
			u16_tempIn = px_drv->rd.pu8_arr[0] << 8;
			u16_tempIn |= px_drv->rd.pu8_arr[1];
			tempInDoor = u16_tempIn * TEMP_LSB_UNIT;
		}
#else
		u8_wr[0] = AS6221_REG_TVAL;
		ret = HAL_I2C_Master_Transmit(p_i2c, TEMP_ADDR_INDOOR, u8_wr, 1, 250);
		if(ret != HAL_OK){
			i2c_tx_failed++;
		}
		else{
			i2c_tx_failed = 0;
		}

		ret = HAL_I2C_Master_Receive(p_i2c, TEMP_ADDR_INDOOR, px_drv->rd.pu8_arr, 2, 250);
		if(ret != HAL_OK){
			i2c_rx_failed++;
		}
		else{
			i2c_rx_failed = 0;
			u16_tempIn = px_drv->rd.pu8_arr[0] << 8;
			u16_tempIn |= px_drv->rd.pu8_arr[1];
			tempInDoor = u16_tempIn * TEMP_LSB_UNIT;
		}
#endif

#else
		if(toggle & 1){
			AS6221_Read(TEMP_ADDR_OUTDOOR, AS6221_REG_TVAL, 2);
		}
		else{
			AS6221_Read(TEMP_ADDR_INDOOR, AS6221_REG_TVAL, 2);
		}
#endif
	}
}


void v_TempOut_Test(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 100)){
		timRef = u32_Tim_1msGet();
		uint8_t rd[8];
		if(HAL_I2C_Mem_Read(p_i2c, ADDR_TEMP_OUTDOOR, AS6221_REG_TVAL, I2C_MEMADD_SIZE_8BIT, rd, 2, 1000) == HAL_OK){
			u16_tempOut = rd[0] << 8;
			u16_tempOut |= rd[1];
			tempOutDoor = u16_tempOut * TEMP_LSB_UNIT;

			LOG_DEBUG("AS6221", "out : %.2f", tempOutDoor);
		}
	}
}























