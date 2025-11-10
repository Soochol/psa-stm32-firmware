#ifndef __JH_LIB_SAVGOL_H
#define __JH_LIB_SAVGOL_H

#include "lib_def.h"
#include "lib_ringbuf.h"

#define WINDOW_WIDTH	(25)

#define SAVGOL_PRECISION	float

void SavizkGolayFilter(uint32_t* dest, uint32_t* src, int srcSize, int wind[], int windSize);

void SavizkGolayFilter_TTT(SAVGOL_PRECISION* smooth, SAVGOL_PRECISION* raw, int srcSize, int wind[], int windSize);

void Savitzky_Golay_smoothing(double* y_series, double* destY, int point_number);




void SavizkGolayFilter_Coeff(int windSize, int order);


void SavizkGolayFilter_Merge(int32_t* smooth, int32_t* raw, int srcSize, int order, int windSize);

#endif


