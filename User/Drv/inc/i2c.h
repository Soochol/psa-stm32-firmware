#ifndef __JH_I2C_H
#define __JH_I2C_H

#include "main.h"
#include "lib_def.h"






void v_I2C_Deinit();

//////////////////////////////////////
//				I2C1				//
//////////////////////////////////////
#define ADDR_TEMP_OUTDOOR	(0x49 << 1)
#define ADDR_TEMP_INDOOR	(0x48 << 1)

#define ADDR_TOF1			(0x29 << 1)

//////////////////////////////////////
//				I2C2				//
//////////////////////////////////////
#define ADDR_FSR_LEFT	(0x48 << 1)
#define ADDR_FSR_RIGHT	(0x49 << 1)

#define ADDR_IMU_LEFT	(0x68 << 1)
#define ADDR_IMU_RIGHT	(0x69 << 1)



//////////////////////////////////////
//				I2C3				//
//////////////////////////////////////
#define ADDR_TOF2		(0x29 << 1)	//0x52
#define ADDR_GPS		(0x42 << 1)	// SAM-M10Q GPS module

//////////////////////////////////////
//				I2C4				//
//////////////////////////////////////
#define ADDR_IR_TEMP	(0x33 << 1)



//////////////////////////////////////
//				I2C5				//
//////////////////////////////////////
//hold off
#define ADDR_CODEC		(0x10 << 1)

//////////////////////////////////////
/*				I2C1				*/
//////////////////////////////////////
int i_I2C1_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
int i_I2C1_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);


//////////////////////////////////////
/*				I2C2				*/
//////////////////////////////////////
int i_I2C2_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
int i_I2C2_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);


//////////////////////////////////////
/*				I2C3				*/
//////////////////////////////////////
int i_I2C3_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
int i_I2C3_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);


//////////////////////////////////////
/*				I2C4				*/
//////////////////////////////////////
int i_I2C4_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len);
int i_I2C4_Read(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len);

int i_I2C4_Config_400KHz();
int i_I2C4_Config_1MHz();

void v_I2C4_Set_Comm_Ready();
int i_I2C4_Write_DMA(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
int i_I2C4_Read_DMA(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);


//////////////////////////////////////
/*				I2C5				*/
//////////////////////////////////////
int i_I2C5_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_data, uint16_t u16_len);
int i_I2C5_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);


void v_I2C2_Bus_Recovery_FastMode(void);
void v_I2C5_Bus_Recovery_FastMode(void);



#endif


