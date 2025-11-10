#include "icm42670p_transport.h"







/*
 * brief	: bank0 read
 * date
 * - create	: 25.05.13
 * - modify	: -
 */
_e_ICM42670P_RET_t _e_ICM42670P_Read_BANK0(_x_ICM42670P_TRANS_t* px_tr, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_arrLen){
	if(px_tr->fn.i_bus() == ICM42670P_RET_BUSY){return ICM42670P_RET_BUSY;}
	px_tr->fn.i_read(u8_reg, pu8_arr, u16_arrLen);
	return ICM42670P_RET_OK;
}

/*
 * brief	: bank0 write
 * date
 * - create	: 25.05.13
 * - modify	: -
 */
_e_ICM42670P_RET_t _e_ICM42670P_Write_BANK0(_x_ICM42670P_TRANS_t* px_tr, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_arrLen){
	if(px_tr->fn.i_bus() == ICM42670P_RET_BUSY){return ICM42670P_RET_BUSY;}
	px_tr->fn.i_write(u8_reg, pu8_arr, u16_arrLen);
	return ICM42670P_RET_OK;
}

/*
 * brief	: mreg read
 * date
 * - create	: 25.05.13
 * - modify	: -
 */
_e_ICM42670P_RET_t _e_ICM42670P_Read_MREG(_x_ICM42670P_TRANS_t* px_tr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_arrLen){
	if(px_tr->fn.i_bus() == ICM42670P_RET_BUSY){return ICM42670P_RET_BUSY;}
	px_tr->fn.i_read(u16_reg, pu8_arr, u16_arrLen);
	//wait for 10us over : 1ms ~ 2ms

	uint8_t arr[3];
	static uint16_t order;

	if(order == 0){
		//mclk(internal clock running) check
	}
	else if(order == 1){

	}

	//Is BLK_SEL_R value 0?
	arr[0] = (u16_reg & 0xFF00) >> 8;	//BLK_SEL
	arr[1] = u16_reg & 0x00FF;			//REG
	//BLK_SEL_R + MADDR_R	: bank select + register
	px_tr->fn.i_write(ICM42670P_BANK0_REG_BLK_SEL_R, arr, 2);
	//wait for 10us
	px_tr->fn.i_usDelay(10);
	//M_R	: read data
	px_tr->fn.i_read(ICM42670P_BANK0_REG_M_R, arr, 1);
	//wait for 10us
	px_tr->fn.i_usDelay(10);
	//BLK_SEL default
	arr[0] = 0;
	px_tr->fn.i_write(ICM42670P_BANK0_REG_BLK_SEL_W, arr, 1);

	return ICM42670P_RET_OK;
}

/*
 * brief	: mreg write
 * date
 * - create	: 25.05.13
 * - modify	: -
 */
_e_ICM42670P_RET_t _e_ICM42670P_Write_MREG(_x_ICM42670P_TRANS_t* px_tr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_arrLen){
	if(px_tr->fn.i_bus() == ICM42670P_RET_BUSY){return ICM42670P_RET_BUSY;}
	px_tr->fn.i_write(u16_reg, pu8_arr, u16_arrLen);
	return ICM42670P_RET_OK;
}








void Init(){
	/*	configure serial interface	*/

	/*	device reset	*/

	/*	init	*/

}

int _i_ICM422670_ConfigInterface(){
	//BLK_SEL_R = 0
	//BLK_SEL_W = 0
	//if == i2C
	//else SPI
	return 0;
}

int _i_ICM42670P_DeviceReset(){
	//BLK_SEL_R = 0
	//BLK_SEL_W = 0
	//SOFT RESET
	//reset delay : 1ms
	//configure serial interface : _i_ICM422670_ConfigInterface
	//clear reset done interrupt
	return 0;
}

int _i_ICM42670P_Init(){
	//read PWR_MGMT0
	//read GYRO_CONFIG0
	//read ACCEL_CONFIG0
	//read
	return 0;
}















