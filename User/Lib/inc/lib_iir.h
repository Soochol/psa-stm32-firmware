#ifndef __JH_LIB_IIR_H
#define __JH_LIB_IIR_H

#include "lib_def.h"

typedef struct {
	float alpha;
	float out;
} _x_IIR_1ST_t;


void _v_IIR_1st_Init(_x_IIR_1ST_t* px_filt, float alpha);
float _f_IIR_1st_Upd(_x_IIR_1ST_t* px_filt, float in);




typedef double REAL;
REAL applyfilter(REAL v);


#endif


