#include "lib_lpf.h"

#include <stdio.h>


void v_T1IIR_Init(x_T1IIR_t* px_filter, float f_a){
	if(f_a < 0.0f){
		px_filter->a = 0.0f;
	}
	else if(f_a > 1.0f){
		px_filter->a = 1.0f;
	}
	else{
		px_filter->a = f_a;
	}
	px_filter->y = 0.0f;
}

float f_T1IIR_New(x_T1IIR_t* px_filter, float f_x){
	px_filter->y = (1.0f - px_filter->a) * f_x + px_filter->a * px_filter->y;
	return px_filter->y;
}




