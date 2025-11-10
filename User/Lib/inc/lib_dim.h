#ifndef __JH_LIB_DIM_H
#define __JH_LIB_DIM_H

#include "lib_def.h"


typedef enum {_dimUP=1, _dimDOWN, _dimUPDOWN} _e_DIM_t;
typedef struct {
	struct {
		uint32_t* pu32_tick;	//
		uint32_t u32_ref_itv;
		uint32_t u32_itv;		//
		uint32_t u32_ref_keep;
		uint32_t u32_keep;		//
	}tim;
	struct {
		uint32_t u32_max;		//
		uint32_t u32_min;		//
		uint32_t u32_idx;
		_e_DIM_t e_val_dir;		//
		_e_DIM_t e_now_dir;		//start direction, when direction is up-down.
	}dim;
} _x_DIM_CFG_t;

void _v_Dim_CFG(_x_DIM_CFG_t* px_cfg, uint32_t* pu32_tick, uint32_t u32_itv, uint32_t u32_keep,\
			   uint32_t u32_max, uint32_t u32_min, _e_DIM_t e_dir);
uint32_t _u32_Dim_Get_IDX(_x_DIM_CFG_t* px_cfg);
bool _b_Dim_Is_On(_x_DIM_CFG_t* px_cfg, uint32_t u32_idx);

#endif


