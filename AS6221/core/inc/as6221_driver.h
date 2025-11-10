#ifndef __JH_AS6221_DRIVER_H
#define __JH_AS6221_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "as6221_regmag.h"
#include "as6221_transport.h"

typedef struct {
	_x_AS6221_TRANS_t	tr;
	struct {
		uint8_t* pu8_arr;
		uint16_t u16_cnt;
	}rd;
} _AS6221_DRV_t;

typedef enum {
	AS6221_RET_OK=0,
	AS6221_RET_BUSY,
	AS6221_RET_DELAY,
	AS6221_RET_ERR_SIZE,
	AS6221_RET_ERR_ARG,
	AS6221_RET_ERR_DRV,
} _e_AS6221_RET_t;



int _i_AS6221_Get_Temp(_AS6221_DRV_t* px_drv);


#ifdef __cplusplus
}
#endif

#endif


