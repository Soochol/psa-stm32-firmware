#include "icm42670p_driver.h"


#if 0


static int i_ICM42670P_Proc(_x_ICM42670P_DRV_t* px_drv);


/*
 * brief	: process handler
 * date
 * - create	: 25.05.19
 * - modify	: -
 * note
 * - place in read or write done handler
 */
int i_ICM42670P_ProcHandler(_x_ICM42670P_DRV_t* px_drv){
	_e_ICM42670P_RET_t ret;
	ret = i_ICM42670P_Proc(px_drv);
	return ret;
}


void v_ICM42670P_RdArrInit(_x_ICM42670P_DRV_t* px_drv, uint8_t* pu8_arr, uint16_t u16_arrMax){
	//if heap..
	px_drv->arr.rd.pu8 = pu8_arr;
	px_drv->arr.rd.u16_max = u16_arrMax;
	px_drv->arr.rd.u16_cnt = 0;
}


void v_ICM42670P_WrArrInit(_x_ICM42670P_DRV_t* px_drv, uint8_t* pu8_arr, uint16_t u16_arrMax){
	//if busy.. block defense
	px_drv->arr.wr.pu8 = pu8_arr;
	px_drv->arr.wr.u16_max = u16_arrMax;
	px_drv->arr.wr.u16_cnt = 0;
}


static int i_ICM42670P_Proc(_x_ICM42670P_DRV_t* px_drv){
	_e_ICM42670P_RET_t ret;

	return ret;
}


/*
 * brief	: initialize
 * date
 * - create	: 25.05.14
 * - modify	: -
 */
int i_ICM42670P_Init(_x_ICM42670P_DRV_t* px_drv){

}

/*
 * brief	: soft reset
 * date
 * - create	: 25.05.14
 * - modify	: -
 */
int i_ICM42670P_SoftReset(_x_ICM42670P_DRV_t* px_drv){

	//proc -> proc (X)

	//order copy



}


void v_ICM42670P_Init_HandlerIdx(_x_ICM42670P_DRV_t* px_drv, int (*i_fn)(_x_ICM42670P_DRV_t*, ...)){
	//parameter mismatch..
	if(px_drv->hd.i_fn[px_drv->hd.u16_idx] != i_fn){

	}



}


int i_ICM42670P_Get_WhoAmI(_x_ICM42670P_DRV_t* px_drv){

	if(px_drv->hd.u16_order[px_drv->hd.u16_idx] == 0){
		px_drv->hd.u16_order[px_drv->hd.u16_idx]++;
		px_drv->tr.fn.i_read(ICM42670P_BANK0_REG_WHO_AM_I, px_drv->arr.rd.pu8, 1);
		return ICM42670P_RET_OK;
	}
	else{
		//done
		px_drv->arr.rd.u16_cnt = 1;

		px_drv->hd.u16_order[px_drv->hd.u16_idx] = 0;
		if(px_drv->hd.u16_idx > 0){
			px_drv->hd.u16_idx--;
			//next
		}
		else{
			px_drv->evt.arg.pu8_arr = px_drv->arr.rd.pu8;
			px_drv->evt.arg.u16_cnt = px_drv->arr.rd.u16_cnt;
			px_drv->evt.v_cb(&px_drv->evt.arg);
		}
		return ICM42670P_RET_OK;
	}
}



/*
 * ACCEL
 */
int i_ICM42670P_Accel_LowPwr(_x_ICM42670P_DRV_t* px_drv){

}

int i_ICM42670P_Accel_LowNoise(_x_ICM42670P_DRV_t* px_drv){

}

//unique?
//disable <-> enable..
//where is parameter?
//sleep mode
int i_ICM42670P_Disable_Accel(_x_ICM42670P_DRV_t* px_drv){

}

//int i_ICM42670P_Set_Accel_Mode();


int i_ICM42670P_Set_Accel_Freq(){

}

int i_ICM42670P_Set_Accel_LowPassAvg(){

}

int i_ICM42670P_Set_Accel_LowNoise_BandWidth(){

}

int i_ICM42670P_Set_Accel_FullScale(){

}

int i_ICM42670P_Get_Accel_FullScale(){

}


/*
 * GYRO
 */
int i_ICM42670P_Gyro_LowNoise(){

}

int i_ICM42670P_Gyro_Disable(){

}

//int i_ICM42670P_Set_Gyro_Mode();

int i_ICM42670P_Set_Gyro_Freq(){

}

int i_ICM42670P_Set_Gyro_LowNoise_BandWidth(){

}

int i_ICM42670P_Set_Gyro_FullScale(){

}

int i_ICM42670P_Get_Gyro_FullScale(){

}


//
//fsync
int i_ICM42670P_Enable_TimeStamp(){

}

int i_ICM42670P_Disable_TimeStamp(){

}


int i_ICM42670P_Set_SPI_SlewRate(){

}


//interrupt pin
int i_ICM42670P_Config_Pin_Int1(){

}

int i_ICM42670P_Config_Pint_Int2(){

}


int i_ICM42670P_Get_Config_Int1(){

}

int i_ICM42670P_Get_Config_Int2(){

}

int i_ICM42670P_Set_Config_Int1(){

}

int i_ICM42670P_Set_Config_Int2(){

}



//data get..
//1. from register
//2. from fifo (if enable)
int i_ICM42670P_Get_Data_FromReg(){

}

int i_ICM42670P_Get_FifoCnt(){

}

//??
int i_ICM42670P_Decode_Fifo(){
	//header decode...

}

int i_ICM42670P_Get_Data_FromFifo(){
	//i_ICM42670P_Decode_Fifo


}



int i_ICM42670P_Set_TimeStamp(){
	//resolution, ..
}


int i_ICM42670P_Reset_Fifo(){

}

int i_ICM42670P_Enable_FifoHighResol(){

}

int i_ICM42670P_Disable_FifoHighResol(){

}


int i_ICM42670P_Config_Fifo(){

}

//Wake On Motion
int i_ICM42670P_Config_Wom(){

}

int i_ICM42670P_Enable_Wom(){

}

int i_ICM42670P_Disable_Wom(){

}

//Digital Motion Processor
int i_ICM42670P_Start_Dmp(){
	//clear by hardware
}

//???
int i_ICM42670P_Resume_Dmp(){

}

int i_ICM42670P_Reset_Dmp(){

}


int i_ICM42670P_Set_Endian(){

}

int i_ICM42670P_Get_Endian(){

}


int i_ICM42670P_Set_FifoDataRate(){

}


//I2C or SPI
int i_ICM42670P_Config_Interface(){

}


//init_hardware





#endif






