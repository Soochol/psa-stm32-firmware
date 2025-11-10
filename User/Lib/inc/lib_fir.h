#ifndef __JH_LIB_FIR_H
#define __JH_LIB_FIR_H

#include "lib_def.h"

#define FIR_FLITER_LEN	(77)

typedef struct {
	float buf[FIR_FLITER_LEN];
	uint8_t bufindex;

	float out;
} _x_FIR_t;


void _v_FIR_Init(_x_FIR_t* fir);
float _f_FIR_Upd(_x_FIR_t* fir, float in);


#endif



