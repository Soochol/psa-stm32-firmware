#ifndef __JH_LIB_LPF_H
#define __JH_LIB_LPF_H

#include "lib_def.h"

typedef struct{
	//alpha 0<a<1, 0 means no filtering
	float a;
	 //output
	float y;
}x_T1IIR_t;

void v_T1IIR_Init(x_T1IIR_t* px_filter, float f_a);
float f_T1IIR_New(x_T1IIR_t* px_filter, float f_x);







#endif


