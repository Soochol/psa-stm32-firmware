#ifndef __JH_LIB_PEAKFIND_H
#define __JH_LIB_PEAKFIND_H

#include "lib_def.h"


#define PEAK_PRECISION_INT32	(0)
#define PEAK_PRECISION_FLOAT	(1)
#define PEAK_PRECISION_DOUBLE	(2)

#define PEAK_PRECISION_TYPE		(PEAK_PRECISION_INT32)


#if PEAK_PRECISION_TYPE == PEAK_PRECISION_FLOAT
#define PEAK_PRECISION	double
#elif PEAK_PRECISION_TYPE == PEAK_PRECISION_FLOAT
#define PEAK_PRECISION	float
#else
#define PEAK_PRECISION	int32_t
#endif






typedef struct {
	PEAK_PRECISION x, y;
} _x_PEAK_XY_t;


typedef struct {
	_x_PEAK_XY_t *arr;	//multiple of 2
	uint16_t u16_in, u16_out, u16_cnt;
	uint16_t u16_mask;
} _x_PEAK_RING_t;


typedef struct {
	uint16_t u16_minX;
	uint16_t u16_maxX;
	uint16_t u16_minY;
	uint16_t u16_maxY;
} _x_PEAK_VALID_t;

void _v_PeakFind(_x_PEAK_VALID_t* px_valid, _x_PEAK_RING_t* px_dst, PEAK_PRECISION* p_src, uint16_t u16_srcCnt);


bool _b_SSF_PeakDetect(PEAK_PRECISION* rawArr, uint16_t u16_ArrCnt, PEAK_PRECISION* peakArr, uint16_t* pu16_peakCnt);



#endif
