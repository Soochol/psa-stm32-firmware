#include "main.h"
#include "mlx90640_platform.h"
#include "i2c.h"
#include "tim.h"
#include "mode.h"
#include "stdio.h"
#include "string.h"
//////////////////////////////////
/*		DRIVER INIT				*/
//////////////////////////////////
static uint8_t u8_wr[16];
static e_COMM_STAT_t e_tempIR_config;

/*
 * skip
 */
void MLX90640_I2CInit(void){

}


/*
 * skip
 */
int MLX90640_I2CGeneralReset(void){
	return 0;
}




int MLX90640_I2CRead(uint8_t slaveAddr,uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data){
	int ret = i_I2C4_Read(slaveAddr, startAddress, (uint8_t*)data, nMemAddressRead);

	uint8_t msb, lsb;
	uint8_t* p = (uint8_t*)data;
	for(uint16_t i=0; i<nMemAddressRead; i++){
		msb = p[i*2];		//MSB
		lsb = p[i*2 + 1];	//LSB
		data[i] = (msb << 8) & 0xFF00;
		data[i] |= (lsb & 0x00FF);
	}

	if(ret == COMM_STAT_OK){
		return 0;
	}
	else{
		return -1;
	}
}


int MLX90640_I2CWrite(uint8_t slaveAddr,uint16_t writeAddress, uint16_t data){
	u8_wr[0] = data >> 8;
	u8_wr[1] = data;
	int ret = i_I2C4_Write(slaveAddr, writeAddress, u8_wr, 1);
	if(ret == COMM_STAT_OK){
		return 0;
	}
	else{
		return -1;
	}
}

/*
 * skip
 */
void MLX90640_I2CFreqSet(int freq){


}


static e_COMM_STAT_t e_tempIR_evt;
static uint16_t u16_rdBuf[100];
static uint32_t u32_toutRef;
uint32_t u32_tempIR_ErrCnt;

void v_Temp_IR_WR_Done(){
	e_tempIR_evt = COMM_STAT_DONE;
}

void v_Temp_IR_RD_Done(uint8_t* pu8, uint16_t u16_cnt){
	e_tempIR_evt = COMM_STAT_DONE;

	uint8_t msb, lsb;
	uint8_t* p = (uint8_t*)pu8;
	uint16_t cnt = u16_cnt >> 1;
	for(uint16_t i=0; i<cnt; i++){
		msb = p[i*2];		//MSB
		lsb = p[i*2 + 1];	//LSB
		u16_rdBuf[i] = (msb << 8) & 0xFF00;
		u16_rdBuf[i] |= (lsb & 0x00FF);
	}
}

int i_Temp_IR_Write_DMA(uint8_t u8_addr, uint16_t u16_memAddr, uint16_t* pu16, uint16_t u16_cnt){
	e_tempIR_evt = COMM_STAT_BUSY;
	u32_toutRef = u32_Tim_1msGet();
	for(uint16_t i=0; i<u16_cnt; i++){
		u8_wr[i*2] = pu16[i] >> 8;
		u8_wr[i*2 + 1] = pu16[i];
	}
	return i_I2C4_Write_DMA(u8_addr, u16_memAddr, u8_wr, u16_cnt << 1);
}

int i_Temp_IR_Read_DMA(uint8_t u8_addr, uint16_t u16_memAddr, uint16_t u16_cnt){
	e_tempIR_evt = COMM_STAT_BUSY;
	u32_toutRef = u32_Tim_1msGet();
	return i_I2C4_Read_DMA(u8_addr, u16_memAddr, u16_cnt << 1);
}

void v_Temp_IR_Tout_Handler(){
	if(u32_tempIR_ErrCnt > 10 || ((e_tempIR_evt == COMM_STAT_BUSY) && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000))){
		//timeout
		e_tempIR_evt = COMM_STAT_READY;
		e_tempIR_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_TEMP_IR);
		v_Mode_SetNext(modeERROR);
	}
}

//////////////////////////////////
/*			TEST				*/
//////////////////////////////////

typedef enum {
	MLX90640_CTRL_IR_REFRESH_RATE_0_5HZ=0,
	MLX90640_CTRL_IR_REFRESH_RATE_1HZ,
	MLX90640_CTRL_IR_REFRESH_RATE_2HZ,
	MLX90640_CTRL_IR_REFRESH_RATE_4HZ,
	MLX90640_CTRL_IR_REFRESH_RATE_8HZ,
	MLX90640_CTRL_IR_REFRESH_RATE_16HZ,
	MLX90640_CTRL_IR_REFRESH_RATE_32HZ,
	MLX90640_CTRL_IR_REFRESH_RATE_64HZ,
} e_MLX90640_CTRL_IR_REFRESH_RATE_t;

#define  TA_SHIFT 8 //Default shift for MLX90640 in open air


paramsMLX90640 mlx90640;
uint16_t eeMLX90640[MLX90640_EEPROM_DUMP_NUM];

paramsMLX90640 mlx1, mlx2;
uint16_t eep1[MLX90640_EEPROM_DUMP_NUM];
uint16_t eep2[MLX90640_EEPROM_DUMP_NUM];

float mlx90640To[768];
uint16_t u16_TempIR_frame[834];
float emissivity=0.95;

float irTemp_vdd;
float irTemp_Ta;
float irTemp_tr;

uint32_t irTemp_retry;

int ret_DumpEE;
int ret_Param;
int ret_Frame;

int ret_DumpEE1, ret_DumpEE2;
int DumpDiff;

int ret_Param1, ret_Param2;

extern int ExtractDeviatingPixels(uint16_t *eeData, paramsMLX90640 *mlx90640);
static inline uint8_t CountBad(uint16_t *list);

uint8_t broken_pix1, broken_pix2;
uint8_t broken_out1, broken_out2;

void v_IR_TEMP_Test(){
	static uint32_t timRef;
	static bool config;
	int status;

	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 5000)){
		timRef = u32_Tim_1msGet();

		if(config == true){
			status = MLX90640_GetFrameData(ADDR_IR_TEMP, u16_TempIR_frame);
			if(status != 0){
				ret_Frame = status;
			}
			irTemp_vdd = MLX90640_GetVdd(u16_TempIR_frame, &mlx90640);
			irTemp_Ta = MLX90640_GetTa(u16_TempIR_frame, &mlx90640);
			irTemp_tr = irTemp_Ta - TA_SHIFT;

			MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels,mlx90640To, 1,&mlx90640);
			MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels,mlx90640To, 1,&mlx90640);
			MLX90640_CalculateTo(u16_TempIR_frame, &mlx90640, emissivity , irTemp_tr, mlx90640To);

			irTemp_retry++;
		}
	}
	if(config == false){
		MLX90640_SetRefreshRate(ADDR_IR_TEMP, MLX90640_CTRL_IR_REFRESH_RATE_16HZ);
		MLX90640_SetChessMode(ADDR_IR_TEMP);
		status = MLX90640_DumpEE(ADDR_IR_TEMP, eeMLX90640);
		if(status != 0){
			ret_DumpEE = status;
		}
		status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
		if(status != 0){
			ret_Param = status;
		}
		config = true;
	}
}




void v_TempIR_Deinit(){
	e_tempIR_config = COMM_STAT_READY;
}

e_COMM_STAT_t e_IR_Temp_Ready(){
	if(e_tempIR_config == COMM_STAT_READY){
		if(MLX90640_SetRefreshRate(ADDR_IR_TEMP, MLX90640_CTRL_IR_REFRESH_RATE_16HZ) != MLX90640_NO_ERROR){
			return e_tempIR_config = COMM_STAT_ERR;
		}
#if 0
		if(MLX90640_SetChessMode(ADDR_IR_TEMP) != MLX90640_NO_ERROR){
			return e_tempIR_config = COMM_STAT_ERR;
		}
#endif
		if(MLX90640_SetInterleavedMode(ADDR_IR_TEMP) != MLX90640_NO_ERROR){
			return e_tempIR_config = COMM_STAT_ERR;
		}
		if(MLX90640_DumpEE(ADDR_IR_TEMP, eeMLX90640) != MLX90640_NO_ERROR){
			return e_tempIR_config = COMM_STAT_ERR;
		}
		if(MLX90640_ExtractParameters(eeMLX90640, &mlx90640) != MLX90640_NO_ERROR){
			return e_tempIR_config = COMM_STAT_ERR;
		}

		//i_I2C4_Config_1MHz();
		e_tempIR_config = COMM_STAT_DONE;

		v_I2C4_Set_Comm_Ready();
	}
	return e_tempIR_config;
}

float f_ir_temp_maxTemp;

uint32_t tempIR_succ, tempIR_fail;

void v_IR_TEMP_Handler(){
	static uint32_t timRef;
	int status;
	static uint32_t tout;
	if(e_tempIR_config != COMM_STAT_DONE){return;}


	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 100)){
		timRef = u32_Tim_1msGet();

		status = MLX90640_GetFrameData(ADDR_IR_TEMP, u16_TempIR_frame);
		if(status != 0){
			if(MLX90640_GetSubPageNumber(u16_TempIR_frame) != 1){
				tout++;
				if(tout > 10){
					e_tempIR_config = COMM_STAT_ERR;
					v_Mode_Set_Error(modeERR_TEMP_IR);
					v_Mode_SetNext(modeERROR);
				}
				tempIR_fail++;
			}
			ret_Frame = status;
			//printf("temp error\n");
			return;
		}
		else{
			tout = 0;
			tempIR_succ++;
			//printf("temp succ\n");
		}
		irTemp_Ta = MLX90640_GetTa(u16_TempIR_frame, &mlx90640);
		irTemp_tr = irTemp_Ta - TA_SHIFT;

		MLX90640_CalculateTo(u16_TempIR_frame, &mlx90640, emissivity , irTemp_tr, mlx90640To);
		MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels,mlx90640To, 1,&mlx90640);
		MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels,mlx90640To, 1,&mlx90640);

		float max_temp = -1000.0f;
		for(int i=0; i<768; i++){
			if(mlx90640To[i] > max_temp){
				max_temp = mlx90640To[i];
			}
		}
		f_ir_temp_maxTemp = max_temp;
	}
}


float f_IR_Temp_Get(){
	return f_ir_temp_maxTemp;
}


static inline uint8_t CountBad(uint16_t *list)
{
    uint8_t n = 0;
    while (n < 5 && list[n] != 0xFFFF) n++;
    return n;        // 0‥4
}



e_COMM_STAT_t e_TempIR_Is_DataReady(int* pi_init, int* pi_rdy){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}

	if(order == 0){
		if(i_Temp_IR_Read_DMA(ADDR_IR_TEMP, MLX90640_STATUS_REG, 1) == COMM_STAT_OK){order++;}
	}
	else{
		*pi_rdy = MLX90640_GET_DATA_READY(u16_rdBuf[0]);
		return COMM_STAT_DONE;
	}
	return COMM_STAT_OK;
}




static int ValidateAuxData(uint16_t *auxData)
{

    if(auxData[0] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;

    for(int i=8; i<19; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }

    for(int i=20; i<23; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }

    for(int i=24; i<33; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }

    for(int i=40; i<51; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }

    for(int i=52; i<55; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }

    for(int i=56; i<64; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }

    return MLX90640_NO_ERROR;
}


static int ValidateFrameData(uint16_t *frameData)
{
    uint8_t line = 0;

    for(int i=0; i<MLX90640_PIXEL_NUM; i+=MLX90640_LINE_SIZE)
    {
        if((frameData[i] == 0x7FFF) && (line%2 == frameData[833])) return -MLX90640_FRAME_DATA_ERROR;
        line = line + 1;
    }

    return MLX90640_NO_ERROR;
}


void v_Temp_IR_Data_Handler(){
	static uint32_t timRef, timItv;
	static uint16_t order;
	static int subpage, read;
	static uint32_t address;

	if(e_tempIR_config != COMM_STAT_DONE){return;}
	if(e_tempIR_evt == COMM_STAT_BUSY){return;}
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){return;}
	if(timItv){
		timRef = u32_Tim_1msGet();
		timItv = 0;
		order = 0;
		read = 0;
	}
	uint16_t wr;

	switch(order){
	case 0:
		//read	MLX90640_STATUS_REG, 1
		if(read){
			if(MLX90640_GET_DATA_READY(u16_rdBuf[0])){
				subpage = MLX90640_GET_FRAME(u16_rdBuf[0]);
				address = MLX90640_PIXEL_DATA_START_ADDRESS + !!subpage * 0x20;
				read = 0;
				order++;
			}
			else{
				timItv = 5;
			}
		}
		else{
			if(i_Temp_IR_Read_DMA(ADDR_IR_TEMP, MLX90640_STATUS_REG, 1) == COMM_STAT_OK){
				read = 1;
			}
		}
		break;
	case 1:
		//read ram
		if(read){
			read = 0;
			memcpy(&u16_TempIR_frame[address - MLX90640_PIXEL_DATA_START_ADDRESS], u16_rdBuf, sizeof(uint16_t) * 0x20);
			address += 0x40;
			if(address >= 0x700){order++;}
		}
		if(order == 1){
			if(i_Temp_IR_Read_DMA(ADDR_IR_TEMP, address, 0x20) == COMM_STAT_OK){
				read = 1;
			}
		}
		break;
	case 2:
		//overwrite enable
		wr = MLX90640_INIT_STATUS_VALUE;
		if(i_Temp_IR_Write_DMA(ADDR_IR_TEMP, MLX90640_STATUS_REG, &wr, 1) == COMM_STAT_OK){
			if(subpage == 0){timItv = 62;}	//wait next subpage	//time interval sample rate
			else			{order++;}
		}
		break;
	case 3:
		//if(subpage == 1)
		//read	MLX90640_AUX_DATA_START_ADDRESS, MLX90640_AUX_NUM(64)
		if(read){
			if(ValidateAuxData(u16_rdBuf) == MLX90640_NO_ERROR){
				for(int cnt=0; cnt<MLX90640_AUX_NUM; cnt++){
					u16_TempIR_frame[cnt+MLX90640_PIXEL_NUM] = u16_rdBuf[cnt];
				}
				order++;
				read = 0;
			}
			else{
				timItv = 62;	//return
				u32_tempIR_ErrCnt++;
			}
		}
		else{
			if(i_Temp_IR_Read_DMA(ADDR_IR_TEMP, MLX90640_AUX_DATA_START_ADDRESS, MLX90640_AUX_NUM) == COMM_STAT_OK){
				read = 1;
			}
		}
		break;
	case 4:
		//read	MLX90640_CTRL_REG, 1
		if(read){
			u16_TempIR_frame[832] = u16_rdBuf[0];
			u16_TempIR_frame[833] = subpage;
			if(ValidateFrameData(u16_TempIR_frame) == MLX90640_NO_ERROR){
				order++;
			}
			else{
				timItv = 62;
				u32_tempIR_ErrCnt++;
			}
		}
		else{
			if(i_Temp_IR_Read_DMA(ADDR_IR_TEMP, MLX90640_CTRL_REG, 1) == COMM_STAT_OK){
				read = 1;
			}
		}
		break;
	case 5:{
		//temperature calculate
		float ta = MLX90640_GetTa(u16_TempIR_frame, &mlx90640);
		float tr = ta - TA_SHIFT;
		MLX90640_CalculateTo(u16_TempIR_frame, &mlx90640, emissivity , tr, mlx90640To);
		MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels,mlx90640To, 1,&mlx90640);
		MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels,mlx90640To, 1,&mlx90640);

		float max_temp = -1000.0f;
		for(int i=0; i<768; i++){
			if(mlx90640To[i] > max_temp){
				max_temp = mlx90640To[i];
			}
		}
		f_ir_temp_maxTemp = max_temp;
		timItv = 62;
		u32_tempIR_ErrCnt = 0;
		break;
	}
	default:
		break;
	}
}



void v_Temp_IR_Test(){
	static int config;
	if(config == 0){
		if(e_IR_Temp_Ready() == COMM_STAT_DONE){
			config = 1;
			v_I2C4_Set_Comm_Ready();
		}
	}
	else{
		v_Temp_IR_Data_Handler();
		v_Temp_IR_Tout_Handler();
	}
}











