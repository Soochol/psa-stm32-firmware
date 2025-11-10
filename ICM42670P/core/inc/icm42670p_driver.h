#ifndef __JH_ICM42670P_DRIVER_H
#define __JH_ICM42670P_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif


#include "icm42670p_def.h"
#include "icm42670p_transport.h"


typedef struct {
	uint8_t* pu8_arr;
	uint16_t u16_max;
	uint16_t u16_len;
} _x_ICM42670P_ARR_t;

typedef enum {
	ICM42670P_EVT_DONE_RD=0,
	ICM42670P_EVT_DONE_WR,
	ICM42670P_EVT_ERR_LEN,
} _e_ICM42670P_EVT_t;

typedef struct {
	uint8_t* pu8_arr;
	uint16_t u16_cnt;
	_e_ICM42670P_EVT_t e_evt;
} _x_ICM42670P_EVT_t;

typedef union {

} _x_ICM42670P_PRM_t;

typedef struct _x_ICM42670P_DRV _x_ICM42670P_DRV_t;
struct _x_ICM42670P_DRV{
	_x_ICM42670P_TRANS_t	tr;

	//done callback
	struct {
		void (*v_cb)(_x_ICM42670P_EVT_t* px_evt);
		_x_ICM42670P_EVT_t arg;
	}evt;

	//common
	struct {
		struct {
			uint8_t* pu8;
			uint16_t u16_max;
			uint16_t u16_cnt;
		}rd;
		struct {
			uint8_t* pu8;
			uint16_t u16_max;
			uint16_t u16_cnt;
		}wr;
	}arr;


	//handler
	struct {
		int (*i_fn[8])(_x_ICM42670P_DRV_t* px_drv, ...);
		//etc..






		uint16_t u16_order[8];
		uint16_t u16_idx;
	}hd;

	//static order..
	uint16_t u16_order;	//proc, rd, wr
	struct {
		uint16_t u16_arr[8];
		uint16_t u16_idx;
	}order;
};



int i_ICM42670P_Init(_x_ICM42670P_DRV_t* px_drv);
int i_ICM42670P_ProcHandler(_x_ICM42670P_DRV_t* px_drv);


#ifdef __cplusplus
}
#endif

#endif

