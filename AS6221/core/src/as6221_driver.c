#include "as6221_driver.h"



int _i_AS6221_Write(_AS6221_DRV_t* px_drv, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len){
	if(px_drv->tr.i_bus() == AS6221_RET_BUSY){return AS6221_RET_BUSY;}

	int ret = px_drv->tr.i_write(u8_reg, pu8_arr, u16_len);
	return ret;
}


int _i_AS6221_Read(_AS6221_DRV_t* px_drv, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len){
	if(px_drv->tr.i_bus() == AS6221_RET_BUSY){return AS6221_RET_BUSY;}

	int ret = px_drv->tr.i_read(u8_reg, pu8_arr, u16_len);
	return ret;
}


int _i_AS6221_Get_Temp(_AS6221_DRV_t* px_drv){
	px_drv->rd.u16_cnt = 2;
	int ret = _i_AS6221_Read(px_drv, AS6221_REG_TVAL, px_drv->rd.pu8_arr, px_drv->rd.u16_cnt);
	return ret;
}






