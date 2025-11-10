#ifndef __JH_ICM42670P_TRANSPORT_H
#define __JH_ICM42670P_TRANSPORT_H

#include "icm42670P_regmap.h"



typedef int (*const _i_ICM42670P_Write)(uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_arrLen);
typedef int (*const _i_ICM42670P_Read)(uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_arrLen);
typedef int (*const _i_ICM42670P_Bus)(void);
typedef int (*const _i_ICM42670P_Delay)(uint16_t u16_delay);
typedef int (*const _i_ICM42670P_Proc)(void);


typedef struct {
	_i_ICM42670P_Write	i_write;
	_i_ICM42670P_Read	i_read;
	_i_ICM42670P_Bus	i_bus;
	_i_ICM42670P_Delay	i_usDelay;	//fixed : only polling
	_i_ICM42670P_Delay	i_msDelay;	//interrupt or polling
	_i_ICM42670P_Proc	i_proc;
} _x_ICM42670P_FN_t;

typedef struct {
	_x_ICM42670P_FN_t fn;



} _x_ICM42670P_TRANS_t;

typedef enum {
	ICM42670P_RET_OK=0,
	ICM42670P_RET_BUSY,
	ICM42670P_RET_DELAY,
	ICM42670P_RET_ERR_SIZE,
	ICM42670P_RET_ERR_ARG,

} _e_ICM42670P_RET_t;





#endif


