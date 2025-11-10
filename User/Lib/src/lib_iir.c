#include "lib_iir.h"




void _v_IIR_1st_Init(_x_IIR_1ST_t* px_filt, float alpha){
	if(alpha < 0.0f){
		px_filt->alpha = 0.0f;
	}
	else if(alpha > 1.0f){
		px_filt->alpha = 1.0f;
	}
	else{
		px_filt->alpha = alpha;
	}

	px_filt->out = 0.0f;
}

float _f_IIR_1st_Upd(_x_IIR_1ST_t* px_filt, float in){
	px_filt->out = (1.0f - px_filt->alpha) * in + px_filt->alpha * px_filt->out;

	return px_filt->out;
}




#define NPOLE 8
#define NZERO 8
REAL acoeff[]={0.99254427173396,-7.946983439401197,27.838466626156848,-55.72663079910892,69.72247621135375,-55.8309882163381,27.9428285985358,-7.991713252932137,1};
REAL bcoeff[]={1,0,-4,0,6,0,-4,0,1};
REAL gain=8007046836.551489;
REAL xv[]={0,0,0,0,0,0,0,0,0};
REAL yv[]={0,0,0,0,0,0,0,0,0};

REAL applyfilter(REAL v)
{
	int i;
	REAL out=0;
	for (i=0; i<NZERO; i++) {xv[i]=xv[i+1];}
	xv[NZERO] = v/gain;
	for (i=0; i<NPOLE; i++) {yv[i]=yv[i+1];}
	for (i=0; i<=NZERO; i++) {out+=xv[i]*bcoeff[i];}
	for (i=0; i<NPOLE; i++) {out-=yv[i]*acoeff[i];}
	yv[NPOLE]=out;
	return out;
}












