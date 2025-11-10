#include "spi.h"
#include "stdio.h"
#include "string.h"

#include "es8388_platform.h"
#include "lib_commbuf.h"

/*
 * AUDIO Interface
 */

#if 0

extern SPI_HandleTypeDef hspi4;
extern DMA_HandleTypeDef hdma_spi4_tx;


static SPI_HandleTypeDef* p_spi = &hspi4;



#define SPI4_RD_SIZE	32
#define SPI4_WR_SIZE	32

#if 0
ALIGN_32BYTES(uint8_t u8_spi4_rdArr[SPI4_RD_SIZE] __attribute__((section(".my_d2_section"))));
ALIGN_32BYTES(uint8_t u8_spi4_wrArr[SPI4_WR_SIZE] __attribute__((section(".my_d2_section"))));

#else

uint8_t u8_spi4_rdArr[SPI4_RD_SIZE];
uint8_t u8_spi4_wrArr[SPI4_WR_SIZE];

#endif

static e_COMM_STAT_t e_comm_spi4;

static uint8_t u8_spi4_addr;


#define SPI4_ADDR_MASK	0xFE


#define SPI4_BUF_SIZE	32
#define SPI4_BUF_MASK	(SPI4_BUF_SIZE - 1)

_x_COMM_BUF_t x_buf[SPI4_BUF_SIZE];
static uint16_t u16_bufIn, u16_bufOut, u16_bufCnt;

#define SPI4_ARR_SIZE	64
static uint8_t u8_arr[SPI4_ARR_SIZE];
static uint16_t u16_arrOut;



static uint16_t u16_num;
static uint16_t u16_actNum;

uint16_t u16_SPI4_BufGet_EmptyCnt(){
	return SPI4_BUF_SIZE - u16_bufCnt;
}

e_COMM_STAT_t e_SPI4_BufIn(uint8_t u8_addr, uint8_t u8_reg, uint8_t* pu8_data, uint16_t u16_len, uint16_t u16_num){
	if(u16_bufCnt < SPI4_BUF_SIZE){
		u16_bufCnt++;
	}
	else{
		return COMM_STAT_FIFO_FULL;
		//u16_bufOut = (u16_bufOut + 1) & SPI4_BUF_MASK;
	}
	//addr
	x_buf[u16_bufIn].u8_addr = u8_addr;
	//reg
	x_buf[u16_bufIn].u8_reg = u8_reg;
	//len
	x_buf[u16_bufIn].u16_cnt = u16_len;
	//data
	if(u16_arrOut + u16_len < SPI4_ARR_SIZE){
		x_buf[u16_bufIn].pu8_data = u8_arr + u16_arrOut;
		u16_arrOut += u16_len;
	}
	else{
		x_buf[u16_bufIn].pu8_data = u8_arr;
		u16_arrOut = u16_len;
	}
	memcpy(x_buf[u16_bufIn].pu8_data, pu8_data, u16_len);
	x_buf[u16_bufIn].u16_num = u16_num;

	u16_bufIn = (u16_bufIn + 1) & SPI4_BUF_MASK;
	return COMM_STAT_OK;
}

_x_COMM_BUF_t x_SPI4_BufOut(){
	_x_COMM_BUF_t buf = {0,};
	if(u16_bufCnt){
		u16_bufCnt--;
		memcpy(&buf, &x_buf[u16_bufOut], sizeof(_x_COMM_BUF_t));
		u16_bufOut = (u16_bufOut + 1) & SPI4_BUF_MASK;
	}
	return buf;
}

#define SPI4_POLLING	1

int i_SPI4_Write_T(uint8_t u8_addr, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len){
	u8_spi4_addr = u8_addr & SPI4_ADDR_MASK;

	u8_spi4_wrArr[0] = u8_spi4_addr;
	u8_spi4_wrArr[1] = u8_reg;
	memcpy(u8_spi4_wrArr + 2, pu8_arr, u16_len);
#if SPI4_POLLING
	return HAL_SPI_Transmit(p_spi, u8_spi4_wrArr, u16_len + 2, HAL_MAX_DELAY);
#else
	//cache push to main memory
	SCB_CleanDCache_by_Addr((uint32_t*)u8_spi4_wrArr, SPI4_WR_SIZE);
	__DSB();
	__DMB();
	return HAL_SPI_Transmit_DMA(p_spi, u8_spi4_wrArr, u16_len + 2);
#endif
}



int i_SPI4_Write(uint8_t u8_addr, uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len, bool b_cont){
#if SPI4_POLLING
	if(e_comm_spi4 == COMM_STAT_READY || e_comm_spi4 == COMM_STAT_OK){
#else
	if(e_comm_spi4 == COMM_STAT_READY){
#endif
		if(u16_len + 1 < SPI4_WR_SIZE){
			HAL_StatusTypeDef ret = i_SPI4_Write_T(u8_addr, u8_reg, pu8_arr, u16_len);
			if(ret == HAL_OK)		{
				e_comm_spi4 = COMM_STAT_OK;
				u16_actNum = u16_num;
				if(b_cont == false)	{u16_num++;}
			}
			else{
				if(ret == HAL_BUSY)	{e_comm_spi4 = COMM_STAT_BUSY;}
				else				{e_comm_spi4 = COMM_STAT_ERR;}
			}
		}
		else{
			e_comm_spi4 = COMM_STAT_ERR_LEN;
		}
	}
	else{
		//queue..
		e_comm_spi4 = e_SPI4_BufIn(u8_addr, u8_reg, pu8_arr, u16_len, u16_num);
		if(b_cont == false)	{u16_num++;}
	}
	return e_comm_spi4;
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi){
	if(hspi == p_spi){
		//write done
		e_comm_spi4 = COMM_STAT_READY;
		if(u16_bufCnt){
			_x_COMM_BUF_t buf = x_SPI4_BufOut();

			if(u16_actNum != buf.u16_num){
				v_AUDIO_Ctrl_WR_Done();
			}

			HAL_StatusTypeDef ret = i_SPI4_Write_T(buf.u8_addr, buf.u8_reg, buf.pu8_data, buf.u16_cnt);
			if(ret == HAL_OK)		{
				e_comm_spi4 = COMM_STAT_OK;
				u16_actNum = u16_num;
			}
			else{
				if(ret == HAL_BUSY)	{e_comm_spi4 = COMM_STAT_BUSY;}
				else				{e_comm_spi4 = COMM_STAT_ERR;}
			}
		}
		else{
			v_AUDIO_Ctrl_WR_Done();
		}
	}
}

#endif




