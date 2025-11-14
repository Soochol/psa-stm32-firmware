#include "i2c.h"
#include "string.h"
#include "lib_commbuf.h"
#include "tim.h"
//platform
#include "as6221_platform.h"
#include "ads111x_platform.h"
#include "icm42670p_platform.h"
#include "mlx90640_platform.h"
#include "es8388_platform.h"
#include "sam_m10q_platform.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c4;


//temp - audio i/f
extern I2C_HandleTypeDef hi2c5;


/*
 * I2C1
 * - Actuator TOF1 			(ADDR : 0x52)
 * - Board Temperature		(ADDR : 0x48)
 * - Outdoor Temperature	(ADDR : 0x49)
 * I2C2
 * - IMU - Right	(ADDR : 0x69)
 * - IMU - Left		(ADDR : 0x68)
 * - FSR - Right	(ADDR : 0x49)
 * - FSR - Left		(ADDR : 0x48)
 * I2C3
 * - Object detect (used?)	(ADDR : 0x52)
 * I2C4
 * - Actuator Temperature	(ADDR : 0x33)
 * I2C5 (temporary) -> change to SPI
 * - Audio Interface
 */

static I2C_HandleTypeDef* p_i2c1 = &hi2c1;
static I2C_HandleTypeDef* p_i2c2 = &hi2c2;
static I2C_HandleTypeDef* p_i2c3 = &hi2c3;
static I2C_HandleTypeDef* p_i2c4 = &hi2c4;

//temp - audio i/f
static I2C_HandleTypeDef* p_i2c5 = &hi2c5;





#define I2C_COMM_ARR_SIZE	32
#define I2C_COMM_ARR_MASK	(I2C_COMM_ARR_SIZE - 1)
#define I2C_BUF_ARR_SIZE	128

typedef struct {
	uint8_t u8_addr;
	uint16_t u16_reg;
	uint8_t* pu8_arr;
	uint16_t u16_len;
	bool b_rd;
} x_I2C_COMM_t;

typedef struct {
	x_I2C_COMM_t comm[I2C_COMM_ARR_SIZE];
	uint8_t u8_arr[I2C_BUF_ARR_SIZE];
	uint16_t u16_arrOut;
	uint16_t u16_in, u16_out, u16_cnt;
	uint16_t u16_max;
	uint16_t u16_ovf;
} x_I2C_BUF_t;


int i_I2C_BufIn(x_I2C_BUF_t* px, uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len, bool b_rd){
	// CRITICAL FIX: Validate buffer length to prevent overflow
	if(u16_len > I2C_BUF_ARR_SIZE){
		return COMM_STAT_ERR_LEN;  // Buffer overflow protection
	}

	px->comm[px->u16_in].u8_addr = u8_addr;
	px->comm[px->u16_in].u16_reg = u16_reg;
	px->comm[px->u16_in].u16_len = u16_len;
	if(b_rd == false){
		if(px->u16_arrOut + u16_len < I2C_BUF_ARR_SIZE){
			px->comm[px->u16_in].pu8_arr = px->u8_arr + px->u16_arrOut;
			px->u16_arrOut += u16_len;
		}
		else{
			// Buffer wrap-around to beginning (validated above)
			px->comm[px->u16_in].pu8_arr = px->u8_arr;
			px->u16_arrOut = u16_len;
		}
		memcpy(px->comm[px->u16_in].pu8_arr, pu8_arr, u16_len);
	}
	px->comm[px->u16_in].b_rd = b_rd;
	px->u16_in = (px->u16_in + 1) & I2C_COMM_ARR_MASK;
	if(px->u16_cnt < I2C_COMM_ARR_SIZE){
		px->u16_cnt++;
		return COMM_STAT_OK;
	}
	else{
		px->u16_out = (px->u16_out + 1) & I2C_COMM_ARR_MASK;
		px->u16_ovf++;
		return COMM_STAT_FIFO_FULL;
	}
}


x_I2C_COMM_t x_I2C_BufOut(x_I2C_BUF_t* px){
	x_I2C_COMM_t comm = {0,};
	if(px->u16_cnt){
		px->u16_cnt--;
		memcpy(&comm, &px->comm[px->u16_out], sizeof(x_I2C_COMM_t));
		px->u16_out = (px->u16_out + 1) & I2C_COMM_ARR_MASK;
	}
	return comm;
}




void v_I2C_Deinit(){
	HAL_I2C_DeInit(p_i2c1);
	HAL_I2C_DeInit(p_i2c2);
	HAL_I2C_DeInit(p_i2c3);
	HAL_I2C_DeInit(p_i2c4);
	HAL_I2C_DeInit(p_i2c5);
}

//////////////////////////////////////
/*				I2C1				*/
//////////////////////////////////////
#define I2C1_RD_SIZE	32
#define I2C1_WR_SIZE	32

static uint8_t u8_i2c1_wrArr[I2C1_WR_SIZE] __attribute__((section(".my_nocache_section")));
static uint8_t u8_i2c1_rdArr[I2C1_RD_SIZE] __attribute__((section(".my_nocache_section")));

// CRITICAL FIX: volatile to prevent race conditions with ISR
static volatile e_COMM_STAT_t e_comm_i2c1;
static uint8_t u8_i2c1_addr;
static uint16_t u16_i2c1_rdCnt;
static x_I2C_BUF_t i2c1_buf;



int i_I2C1_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

	if(e_comm_i2c1 == COMM_STAT_READY){
		if(u16_len + 1 < I2C1_WR_SIZE){
			u8_i2c1_addr = u8_addr;
			memcpy(u8_i2c1_wrArr, pu8_arr, u16_len);
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_DMA(p_i2c1, u8_i2c1_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c1_wrArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c1 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c1 = COMM_STAT_BUSY;}
				else				{e_comm_i2c1 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c1 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c1 = i_I2C_BufIn(&i2c1_buf, u8_addr, u16_reg, pu8_arr, u16_len, false);
	}
	return e_comm_i2c1;
}



int i_I2C1_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len){
	if(e_comm_i2c1 == COMM_STAT_READY){
		if(I2C1_WR_SIZE > 0 && I2C1_RD_SIZE >= u16_len){
			u8_i2c1_addr = u8_addr;
			u16_i2c1_rdCnt = u16_len;
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_DMA(p_i2c1, u8_i2c1_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c1_rdArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c1 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c1 = COMM_STAT_BUSY;}
				else				{e_comm_i2c1 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c1 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c1 = i_I2C_BufIn(&i2c1_buf, u8_addr, u16_reg, NULL, u16_len, true);
	}
	return e_comm_i2c1;
}




//////////////////////////////////////
/*				I2C2				*/
//////////////////////////////////////
#define I2C2_RD_SIZE	32
#define I2C2_WR_SIZE	32

static uint8_t u8_i2c2_wrArr[I2C2_WR_SIZE] __attribute__((section(".my_nocache_section")));
static uint8_t u8_i2c2_rdArr[I2C2_RD_SIZE] __attribute__((section(".my_nocache_section")));


// CRITICAL FIX: volatile to prevent race conditions with ISR
static volatile e_COMM_STAT_t e_comm_i2c2;
static uint8_t u8_i2c2_addr;
static uint16_t u16_i2c2_rdCnt;
static x_I2C_BUF_t i2c2_buf;


int i_I2C2_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

	if(e_comm_i2c2 == COMM_STAT_READY){
		if(u16_len + 1 < I2C2_WR_SIZE){
			u8_i2c2_addr = u8_addr;
			memcpy(u8_i2c2_wrArr, pu8_arr, u16_len);
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_DMA(p_i2c2, u8_i2c2_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c2_wrArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c2 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c2 = COMM_STAT_BUSY;}
				else				{e_comm_i2c2 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c2 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c2 = i_I2C_BufIn(&i2c2_buf, u8_addr, u16_reg, pu8_arr, u16_len, false);
	}
	return e_comm_i2c2;
}


int i_I2C2_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len){
	if(e_comm_i2c2 == COMM_STAT_READY){
		if(I2C2_WR_SIZE > 0 && I2C2_RD_SIZE >= u16_len){
			u8_i2c2_addr = u8_addr;
			u16_i2c2_rdCnt = u16_len;
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_DMA(p_i2c2, u8_i2c2_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c2_rdArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c2 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c2 = COMM_STAT_BUSY;}
				else				{e_comm_i2c2 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c2 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c2 = i_I2C_BufIn(&i2c2_buf, u8_addr, u16_reg, NULL, u16_len, true);
	}
	return e_comm_i2c2;
}



//////////////////////////////////////
/*				I2C3				*/
//////////////////////////////////////
#define I2C3_RD_SIZE	64
#define I2C3_WR_SIZE	64

static uint8_t u8_i2c3_wrArr[I2C3_WR_SIZE] __attribute__((section(".my_nocache_section")));
static uint8_t u8_i2c3_rdArr[I2C3_RD_SIZE] __attribute__((section(".my_nocache_section")));

// CRITICAL FIX: volatile to prevent race conditions with ISR
static volatile e_COMM_STAT_t e_comm_i2c3;
static uint8_t u8_i2c3_addr;
static uint16_t u16_i2c3_rdCnt;
static x_I2C_BUF_t i2c3_buf;


int i_I2C3_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

	if(e_comm_i2c3 == COMM_STAT_READY){
		if(u16_len + 1 < I2C3_WR_SIZE){
			u8_i2c3_addr = u8_addr;
			memcpy(u8_i2c3_wrArr, pu8_arr, u16_len);
			// I2C3 uses IT mode (not DMA)
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_IT(p_i2c3, u8_i2c3_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c3_wrArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c3 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c3 = COMM_STAT_BUSY;}
				else				{e_comm_i2c3 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c3 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c3 = i_I2C_BufIn(&i2c3_buf, u8_addr, u16_reg, pu8_arr, u16_len, false);
	}
	return e_comm_i2c3;
}


int i_I2C3_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len){
	if(e_comm_i2c3 == COMM_STAT_READY){
		if(I2C3_WR_SIZE > 0 && I2C3_RD_SIZE >= u16_len){
			u8_i2c3_addr = u8_addr;
			u16_i2c3_rdCnt = u16_len;
			// I2C3 uses IT mode (not DMA)
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_IT(p_i2c3, u8_i2c3_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c3_rdArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c3 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c3 = COMM_STAT_BUSY;}
				else				{e_comm_i2c3 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c3 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c3 = i_I2C_BufIn(&i2c3_buf, u8_addr, u16_reg, NULL, u16_len, true);
	}
	return e_comm_i2c3;
}


//////////////////////////////////////
/*				I2C4				*/
//////////////////////////////////////
// CRITICAL FIX: volatile to prevent race conditions with ISR
static volatile e_COMM_STAT_t e_comm_i2c4;

uint32_t i2c4_ok, i2c4_busy, i2c4_err;
bool b_i2c4_rdDone, b_i2c4_wrDone;
#define I2C4_CHG	0
int i_I2C4_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_data == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

#if I2C4_CHG
	HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_DMA(p_i2c4, u8_addr, u16_reg, I2C_MEMADD_SIZE_16BIT, pu8_data, u16_len);
#else
	HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(p_i2c4, u8_addr, u16_reg, I2C_MEMADD_SIZE_16BIT, pu8_data, u16_len * 2, 10);
#endif
	if(ret == HAL_OK)		{e_comm_i2c4 = COMM_STAT_OK;}
	else{
		if(ret == HAL_BUSY)	{e_comm_i2c4 = COMM_STAT_BUSY;}
		else				{e_comm_i2c4 = COMM_STAT_ERR;}
	}

	//timeout set && done event wait
	return e_comm_i2c4;
}


int i_I2C4_Read(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_data == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

#if I2C4_CHG
	HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_DMA(p_i2c4, u8_addr, u16_reg, I2C_MEMADD_SIZE_16BIT, pu8_data, u16_len);
#else
	HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(p_i2c4, u8_addr, u16_reg, I2C_MEMADD_SIZE_16BIT, pu8_data, u16_len * 2, HAL_MAX_DELAY);
#endif
	if(ret == HAL_OK)		{e_comm_i2c4 = COMM_STAT_OK;}
	else{
		if(ret == HAL_BUSY)	{e_comm_i2c4 = COMM_STAT_BUSY;}
		else				{e_comm_i2c4 = COMM_STAT_ERR;}
	}
	return e_comm_i2c4;
}

uint16_t i2c4_config_err;
int i_I2C4_Config_400KHz(){
	HAL_I2C_DeInit(p_i2c4);

	p_i2c4->Init.Timing = 0x00C0216C;
	// CRITICAL: Propagate configuration errors instead of silently failing
	if(HAL_I2C_Init(p_i2c4) != HAL_OK){
		i2c4_config_err++;
		return COMM_STAT_ERR;
	}
	if(HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK){
		i2c4_config_err++;
		return COMM_STAT_ERR;
	}
	if(HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK){
		i2c4_config_err++;
		return COMM_STAT_ERR;
	}
	return COMM_STAT_OK;
}

int i_I2C4_Config_1MHz(){
	HAL_I2C_DeInit(p_i2c4);

	p_i2c4->Init.Timing = 0x00B11B24;
	// CRITICAL: Propagate configuration errors instead of silently failing
	if(HAL_I2C_Init(p_i2c4) != HAL_OK){
		i2c4_config_err++;
		return COMM_STAT_ERR;
	}
	if(HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK){
		i2c4_config_err++;
		return COMM_STAT_ERR;
	}
	if(HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK){
		i2c4_config_err++;
		return COMM_STAT_ERR;
	}
	return COMM_STAT_OK;
}






// e_comm_i2c4 already declared above (line 268) - removed duplicate
static uint8_t u8_i2c4_addr;
static uint16_t u16_i2c4_rdCnt;

#define I2C4_RD_SIZE	256
#define I2C4_WR_SIZE	32

static uint8_t u8_i2c4_wrArr[I2C4_WR_SIZE] __attribute__((section(".my_nocache_d3")));
static uint8_t u8_i2c4_rdArr[I2C4_RD_SIZE] __attribute__((section(".my_nocache_d3")));

void v_I2C3_Set_Comm_Ready(){
	e_comm_i2c3 = COMM_STAT_READY;
}

void v_I2C4_Set_Comm_Ready(){
	e_comm_i2c4 = COMM_STAT_READY;
}

int i_I2C4_Write_DMA(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

	if(e_comm_i2c4 == COMM_STAT_READY){
		if(u16_len + 1 < I2C4_WR_SIZE){
			u8_i2c4_addr = u8_addr;
			memcpy(u8_i2c4_wrArr, pu8_arr, u16_len);
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_DMA(p_i2c4, u8_i2c4_addr, u16_reg, I2C_MEMADD_SIZE_16BIT, u8_i2c4_wrArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c4 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c4 = COMM_STAT_BUSY;}
				else				{e_comm_i2c4 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c4 = COMM_STAT_ERR_LEN;
		}
	}
	return e_comm_i2c4;
}


int i_I2C4_Read_DMA(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len){
	if(e_comm_i2c4 == COMM_STAT_READY){
		if(I2C4_WR_SIZE > 0 && I2C4_RD_SIZE >= u16_len){
			u8_i2c4_addr = u8_addr;
			u16_i2c4_rdCnt = u16_len;
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_DMA(p_i2c4, u8_i2c4_addr, u16_reg, I2C_MEMADD_SIZE_16BIT, u8_i2c4_rdArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c4 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c4 = COMM_STAT_BUSY;}
				else				{e_comm_i2c4 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c4 = COMM_STAT_ERR_LEN;
		}
	}
	return e_comm_i2c4;
}






//////////////////////////////////////
/*				I2C5				*/
//////////////////////////////////////
#define I2C5_CHG	1

// CRITICAL FIX: volatile to prevent race conditions with ISR
static volatile e_COMM_STAT_t e_comm_i2c5;

#if I2C5_CHG

#define I2C5_RD_SIZE	32
#define I2C5_WR_SIZE	32

static uint8_t u8_i2c5_wrArr[I2C5_WR_SIZE] __attribute__((section(".my_nocache_section")));
static uint8_t u8_i2c5_rdArr[I2C5_RD_SIZE] __attribute__((section(".my_nocache_section")));



static uint8_t u8_i2c5_addr;
static uint16_t u16_i2c5_rdCnt;
static x_I2C_BUF_t i2c5_buf;


int i_I2C5_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_len == 0){
		return COMM_STAT_ERR;
	}

	if(e_comm_i2c5 == COMM_STAT_READY){
		if(u16_len + 1 < I2C5_WR_SIZE){
			u8_i2c5_addr = u8_addr;
			memcpy(u8_i2c5_wrArr, pu8_arr, u16_len);
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Write_DMA(p_i2c5, u8_i2c5_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c5_wrArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c5 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c5 = COMM_STAT_BUSY;}
				else				{e_comm_i2c5 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c5 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c5 = i_I2C_BufIn(&i2c5_buf, u8_addr, u16_reg, pu8_arr, u16_len, false);
	}
	return e_comm_i2c5;
}


int i_I2C5_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len){
	if(e_comm_i2c5 == COMM_STAT_READY){
		if(I2C5_WR_SIZE > 0 && I2C5_RD_SIZE >= u16_len){
			u8_i2c5_addr = u8_addr;
			u16_i2c5_rdCnt = u16_len;
			HAL_StatusTypeDef ret = HAL_I2C_Mem_Read_DMA(p_i2c5, u8_i2c5_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, u8_i2c5_rdArr, u16_len);
			if(ret == HAL_OK)		{e_comm_i2c5 = COMM_STAT_OK;}
			else{
				if(ret == HAL_BUSY)	{e_comm_i2c5 = COMM_STAT_BUSY;}
				else				{e_comm_i2c5 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_i2c5 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		e_comm_i2c5 = i_I2C_BufIn(&i2c5_buf, u8_addr, u16_reg, NULL, u16_len, true);
	}
	return e_comm_i2c5;
}


#else

uint32_t i2c5_ok, i2c5_busy, i2c5_err;
int i_I2C5_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len){
	HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(p_i2c5, u8_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, pu8_data, u16_len, HAL_MAX_DELAY);
	if(ret == HAL_OK){
		i2c5_ok++;
		e_comm_i2c5 = COMM_STAT_OK;
	}
	else{
		if(ret == HAL_BUSY){
			i2c5_busy++;
			e_comm_i2c5 = COMM_STAT_BUSY;
		}
		else{
			i2c5_err++;
			e_comm_i2c5 = COMM_STAT_ERR;
		}
	}
	return e_comm_i2c5;
}


int i_I2C5_Read(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len){
	HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(p_i2c5, u8_addr, u16_reg, I2C_MEMADD_SIZE_8BIT, pu8_data, u16_len, HAL_MAX_DELAY);
	if(ret == HAL_OK){
		i2c5_ok++;
		e_comm_i2c5 = COMM_STAT_OK;
	}
	else{
		if(ret == HAL_BUSY){
			i2c5_busy++;
			e_comm_i2c5 = COMM_STAT_BUSY;
		}
		else{
			i2c5_err++;
			e_comm_i2c5 = COMM_STAT_ERR;
		}
	}
	return e_comm_i2c5;
}




#endif



void v_I2C2_Bus_Recovery_FastMode(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. SCL, SDA를 GPIO Output으로 설정
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = I2C2_SDA_SUB_Pin | I2C2_SCL_SUB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(I2C2_SCL_SUB_GPIO_Port, &GPIO_InitStruct);
#if 0
    HAL_GPIO_WritePin(I2C2_SDA_SUB_GPIO_Port, I2C2_SDA_SUB_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(I2C2_SCL_SUB_GPIO_Port, I2C2_SCL_SUB_Pin, GPIO_PIN_RESET);
#endif

    HAL_GPIO_ReadPin(I2C2_SDA_SUB_GPIO_Port, I2C2_SDA_SUB_Pin);
    HAL_GPIO_ReadPin(I2C2_SCL_SUB_GPIO_Port, I2C2_SCL_SUB_Pin);
    // 2. SDA가 LOW 상태인지 확인
    if (HAL_GPIO_ReadPin(I2C2_SDA_SUB_GPIO_Port, I2C2_SDA_SUB_Pin) == GPIO_PIN_RESET)
    {
        // 3. SCL을 9~18회 토글
        for (int i = 0; i < 30; i++)
        {
            HAL_GPIO_WritePin(I2C2_SCL_SUB_GPIO_Port, I2C2_SCL_SUB_Pin, GPIO_PIN_SET);
            delay_us(2);  // 약 2us 지연

            HAL_GPIO_WritePin(I2C2_SCL_SUB_GPIO_Port, I2C2_SCL_SUB_Pin, GPIO_PIN_RESET);
            delay_us(2);
        }

        // 4. STOP 조건 생성: SDA ↑ while SCL ↑
        HAL_GPIO_WritePin(I2C2_SCL_SUB_GPIO_Port, I2C2_SCL_SUB_Pin, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(I2C2_SDA_SUB_GPIO_Port, I2C2_SDA_SUB_Pin, GPIO_PIN_SET);
        delay_us(2);

        //add
        // SDA를 LOW로 만들었다가, SCL이 HIGH일 때 SDA를 HIGH로 올려 STOP 조건 생성
        HAL_GPIO_WritePin(I2C2_SDA_SUB_GPIO_Port, I2C2_SDA_SUB_Pin, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(I2C2_SCL_SUB_GPIO_Port, I2C2_SCL_SUB_Pin, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(I2C2_SDA_SUB_GPIO_Port, I2C2_SDA_SUB_Pin, GPIO_PIN_SET);  // STOP 조건
        delay_us(2);
    }

    // 5. 이후 I2C 핀을 Alternate Function으로 재설정
    // (HAL_I2C_Init() 또는 CubeMX 설정으로 복구)

}


GPIO_PinState i2c5_sda, i2c5_scl;
void v_I2C5_Bus_Recovery_FastMode(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. SCL, SDA를 GPIO Output으로 설정
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = I2C5_SDA_AUDIO_Pin | I2C5_SCL_AUDIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(I2C5_SDA_AUDIO_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(I2C5_SDA_AUDIO_GPIO_Port, I2C5_SDA_AUDIO_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(I2C5_SCL_AUDIO_GPIO_Port, I2C5_SCL_AUDIO_Pin, GPIO_PIN_RESET);
#if 0
    i2c5_sda = HAL_GPIO_ReadPin(I2C5_SDA_AUDIO_GPIO_Port, I2C5_SDA_AUDIO_Pin);
    i2c5_scl = HAL_GPIO_ReadPin(I2C5_SCL_AUDIO_GPIO_Port, I2C5_SCL_AUDIO_Pin);
    // 2. SDA가 LOW 상태인지 확인
    if (HAL_GPIO_ReadPin(I2C5_SDA_AUDIO_GPIO_Port, I2C5_SDA_AUDIO_Pin) == GPIO_PIN_RESET)
    {
        // 3. SCL을 9~18회 토글
        for (int i = 0; i < 30; i++)
        {
            HAL_GPIO_WritePin(I2C5_SCL_AUDIO_GPIO_Port, I2C5_SCL_AUDIO_Pin, GPIO_PIN_SET);
            delay_us(2);  // 약 2us 지연

            HAL_GPIO_WritePin(I2C5_SCL_AUDIO_GPIO_Port, I2C5_SCL_AUDIO_Pin, GPIO_PIN_RESET);
            delay_us(2);
        }

        // 4. STOP 조건 생성: SDA ↑ while SCL ↑
        HAL_GPIO_WritePin(I2C5_SCL_AUDIO_GPIO_Port, I2C5_SCL_AUDIO_Pin, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(I2C5_SDA_AUDIO_GPIO_Port, I2C5_SDA_AUDIO_Pin, GPIO_PIN_SET);
        delay_us(2);

        //add
        // SDA를 LOW로 만들었다가, SCL이 HIGH일 때 SDA를 HIGH로 올려 STOP 조건 생성
        HAL_GPIO_WritePin(I2C5_SDA_AUDIO_GPIO_Port, I2C5_SDA_AUDIO_Pin, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(I2C5_SCL_AUDIO_GPIO_Port, I2C5_SCL_AUDIO_Pin, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(I2C5_SDA_AUDIO_GPIO_Port, I2C5_SDA_AUDIO_Pin, GPIO_PIN_SET);  // STOP 조건
        delay_us(2);
    }

    // 5. 이후 I2C 핀을 Alternate Function으로 재설정
    // (HAL_I2C_Init() 또는 CubeMX 설정으로 복구)
#endif
}








//temp : 06.18


///////////////////////////////////////
/*			CALLBACK				*/
//////////////////////////////////////

#define ADS111X_USED

/*
 * brief	: transmit done handler
 * date
 * - create	: 25.05.21
 * - modify	: -
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c){
	//queue

}

/*
 * brief	: recieve done handler
 * date
 * - create	: 25.05.21
 * - modify	: -
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c){
	//queue

}


uint32_t i2c_error;
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c){
	i2c_error++;
	if(hi2c == p_i2c2){

	}
}




void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c){
	if(hi2c == p_i2c1){
		e_comm_i2c1 = COMM_STAT_READY;
		if(u8_i2c1_addr == ADDR_TEMP_INDOOR || u8_i2c1_addr == ADDR_TEMP_OUTDOOR){
			v_AS6221_Write_DoneHandler(u8_i2c1_addr);
		}
		else if(u8_i2c1_addr == ADDR_TOF1){

		}
	}
	else if(hi2c == p_i2c2){
		if(u8_i2c2_addr == ADDR_FSR_LEFT || u8_i2c2_addr == ADDR_FSR_RIGHT){
			e_comm_i2c2 = COMM_STAT_READY;
			v_ADS111X_WrDone(u8_i2c2_addr);
		}
		else if(u8_i2c2_addr == ADDR_IMU_LEFT || u8_i2c2_addr == ADDR_IMU_RIGHT){
			e_comm_i2c2 = COMM_STAT_READY;
			v_IMU_WR_Done(u8_i2c2_addr);
		}
		else{

		}
	}
	else if(hi2c == p_i2c3){
		e_comm_i2c3 = COMM_STAT_READY;
		if(u8_i2c3_addr == ADDR_GPS){
			v_GPS_Write_DoneHandler(u8_i2c3_addr);
		}
	}
	else if(hi2c == p_i2c4){
		e_comm_i2c4 = COMM_STAT_READY;
		v_Temp_IR_WR_Done();
	}
	else if(hi2c == p_i2c5){
		e_comm_i2c5 = COMM_STAT_READY;
		v_Codec_WrDone();
	}


	if(e_comm_i2c1 == COMM_STAT_READY){
		if(i2c1_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c1_buf);
			if(comm.b_rd){
				i_I2C1_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C1_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
	//queue
	if(e_comm_i2c2 == COMM_STAT_READY){
		if(i2c2_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c2_buf);
			if(comm.b_rd){
				i_I2C2_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C2_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
	if(e_comm_i2c3 == COMM_STAT_READY){
		if(i2c3_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c3_buf);
			if(comm.b_rd){
				i_I2C3_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C3_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
	if(e_comm_i2c5 == COMM_STAT_READY){
#if I2C5_CHG
		if(i2c5_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c5_buf);
			if(comm.b_rd){
				i_I2C5_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C5_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
#endif
	}
}


void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c){
	if(hi2c == p_i2c1){
		e_comm_i2c1 = COMM_STAT_READY;
		if(u8_i2c1_addr == ADDR_TEMP_INDOOR || u8_i2c1_addr == ADDR_TEMP_OUTDOOR){
			v_AS6221_Read_DoneHandler(u8_i2c1_addr, u8_i2c1_rdArr, u16_i2c1_rdCnt);
		}
		else if(u8_i2c1_addr == ADDR_TOF1){

		}
		u8_i2c1_addr = 0;
		u16_i2c1_rdCnt = 0;
	}
	else if(hi2c == p_i2c2){
		e_comm_i2c2 = COMM_STAT_READY;

		if(u8_i2c2_addr == ADDR_FSR_LEFT || u8_i2c2_addr == ADDR_FSR_RIGHT){
			v_ADS111X_RdDone(u8_i2c2_addr, u8_i2c2_rdArr, u16_i2c2_rdCnt);
		}
		else if(u8_i2c2_addr == ADDR_IMU_LEFT || u8_i2c2_addr == ADDR_IMU_RIGHT){
			v_IMU_RD_Done(u8_i2c2_addr, u8_i2c2_rdArr, u16_i2c2_rdCnt);
		}
		else{

		}
		u8_i2c2_addr = 0;
		u16_i2c2_rdCnt = 0;
	}
	else if(hi2c == p_i2c3){
		e_comm_i2c3 = COMM_STAT_READY;
		if(u8_i2c3_addr == ADDR_GPS){
			v_GPS_Read_DoneHandler(u8_i2c3_addr, u8_i2c3_rdArr, u16_i2c3_rdCnt);
		}
		u8_i2c3_addr = 0;
		u16_i2c3_rdCnt = 0;
	}
	else if(hi2c == p_i2c4){
		e_comm_i2c4 = COMM_STAT_READY;
		v_Temp_IR_RD_Done(u8_i2c4_rdArr, u16_i2c4_rdCnt);
		u8_i2c4_addr = 0;
		u16_i2c4_rdCnt = 0;
	}
	else if(hi2c == p_i2c5){
#if I2C5_CHG
		e_comm_i2c5 = COMM_STAT_READY;
		v_Codec_RdDone(u8_i2c5_rdArr, u16_i2c5_rdCnt);
		u8_i2c5_addr = 0;
		u16_i2c5_rdCnt = 0;
#endif
	}

	if(e_comm_i2c1 == COMM_STAT_READY){
		if(i2c1_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c1_buf);
			if(comm.b_rd){
				i_I2C1_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C1_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
	//queue
	if(e_comm_i2c2 == COMM_STAT_READY){
		if(i2c2_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c2_buf);
			if(comm.b_rd){
				i_I2C2_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C2_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
	if(e_comm_i2c3 == COMM_STAT_READY){
		if(i2c3_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c3_buf);
			if(comm.b_rd){
				i_I2C3_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C3_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
#if I2C5_CHG
	if(e_comm_i2c5 == COMM_STAT_READY){
		if(i2c5_buf.u16_cnt){
			x_I2C_COMM_t comm = x_I2C_BufOut(&i2c5_buf);
			if(comm.b_rd){
				i_I2C5_Read(comm.u8_addr, comm.u16_reg, comm.u16_len);
			}
			else{
				i_I2C5_Write(comm.u8_addr, comm.u16_reg, comm.pu8_arr, comm.u16_len);
			}
		}
	}
#endif
}






















