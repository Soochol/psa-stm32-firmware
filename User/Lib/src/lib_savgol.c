#include "lib_savgol.h"
#include "math.h"
#include "string.h"






//window
#if WINDOW_WIDTH == 25
int window[] = {-253, -138, -33, 62, 147, 222, 287, 342, 387, 422, 447, 462, 467,\
				462, 447, 422, 387, 342, 287, 222, 147, 62, -33, -138, -253};
#elif WINDOW_WIDTH == 15
int window[] = {-78, -13, 42, 87, 122, 147, 162, 167, \
				162, 147, 122, 87, 42, -13, -78};
#elif WINDOW_WIDTH == 13
int window[] = {-11, 0, 9, 16, 21, 24, 25, \
				24, 21, 16, 9, 0, -11};
#elif WINDOW_WIDTH == 11
int window[] = {-36, 9, 44, 69, 84, 89, \
				84, 69, 44, 9, -36};
#elif WINDOW_WIDTH == 9
int window[] = {-21, 14, 39, 54, 59, \
				54, 39, 14, -21};
#elif WINDOW_WIDTH == 7
int window[] = {-2, 3, 6, 7, \
				6, 3, -2};
#else
int window[] = {-3, -12, 17, \
				12, -3};
#endif



void SavizkGolayFilter(uint32_t* dest, uint32_t* src, int srcSize, int wind[], int windSize){
#if WINDOW_WIDTH
	wind = window;
	windSize = sizeof(window);
#endif
	const int halfWindSize = windSize >> 1;
	windSize = (halfWindSize << 1) + 1;
	uint32_t padded[srcSize + (halfWindSize << 1)];
	//reflective boundary conditions;
	//[hw]...[1][0][1][2]...[hw]...[n-1+hw][n-2+hw][n-3+hw]...[n-1];
	for(int i=0; i<halfWindSize; i++){
		padded[i] = src[halfWindSize - i];
		padded[i + srcSize + halfWindSize] = src[srcSize - 2 - i];
	}
	for(int i = srcSize; i > 0; i--){
		padded[i + halfWindSize] = src[i];
	}
	//
	//double smoothed[srcSize];
	int wsum = 0;
	for(int i=0; i<windSize; i++){
		wsum += wind[i];
	}
	for(int i=srcSize; i > 0; i--){
		uint32_t* ppad = &padded[i];
		double fsum = 0;
		for(int k=0; i<windSize; k++){
			fsum += ppad[k] * wind[k];
		}
		dest[i] = fsum / wsum;
	}
}


void SavizkGolayFilter_TTT(SAVGOL_PRECISION* smooth, SAVGOL_PRECISION* raw, int srcSize, int wind[], int windSize){
#if WINDOW_WIDTH
	wind = window;
	windSize = WINDOW_WIDTH;
#endif
	const int halfWindSize = windSize >> 1;
	windSize = (halfWindSize << 1) + 1;
	SAVGOL_PRECISION padded[srcSize + windSize];
	memset(padded, 0, sizeof(padded));
	//reflective boundary conditions;
	//[hw]...[1][0][1][2]...[hw]...[n-1+hw][n-2+hw][n-3+hw]...[n-1];

	for(int i=0; i<halfWindSize; i++){
		padded[i] = raw[halfWindSize - i];
		padded[i + srcSize + halfWindSize] = raw[srcSize - 2 - i];
	}
	for(int i = srcSize; i >= 0; i--){
		padded[i + halfWindSize] = raw[i];
	}
	//
	//double smoothed[srcSize];
	int wsum = 0;
	for(int i=0; i<windSize; i++){
		wsum += wind[i];
	}
	for(int i=srcSize; i >= 0; i--){
		SAVGOL_PRECISION* ppad = &padded[i];
		SAVGOL_PRECISION fsum = 0;
		for(int k=0; k<windSize; k++){
			fsum += ppad[k] * wind[k];
		}
		smooth[i] = fsum / wsum;
	}
}



void Savitzky_Golay_smoothing(double* y_series, double* destY, int point_number)
{
//        int A[] = { -2, 3, 6, 7, 6, 3, -2 };
//        int A[] = { -21,14,39,54,59,54,39,14,-21};
	int A[] = {-36,9,44,69,84,89,84,69,44,9,-36};
//        int A[] = {-1,9,44,69,84,89,84,69,44,9,-1};

	int n = 5;
	int k = 0;

	for(k = 0; k < n; k++)
	{
		destY[k] = y_series[k] ;
	}

	for(k = n; k < point_number-n; k++)
	{
		int i = 0;
		double nominator = 0;
		double denominator = 0;

		for(i = -n; i <= n; i++)
		{
			nominator += (A[n+i]*y_series[k+i]);
			denominator += A[n+i];
		}
		double y = nominator / denominator;

		if(destY != NULL)  destY[k] = y ;
	}

	for(k = point_number-n ; k < point_number ; k++)
	{
		destY[k] = y_series[k] ;
	}
}












/*  trnm.c    CCMATH mathematics library source code.
  *
  *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
  *  This code may be redistributed under the terms of the GNU library
  *  public license (LGPL). ( See the lgpl.license file for details.)
  * ------------------------------------------------------------------------
  */
 void trnm(double *a, int n)
 {
     double s, *p, *q;

     int i, j, e;

     for (i = 0, e = n - 1; i < n - 1; ++i, --e, a += n + 1) {
         for (p = a + 1, q = a + n, j = 0; j < e; ++j) {
             s = *p;
             *p++ = *q;
             *q = s;
             q += n;
         }
     }
 }

/*  psinv.c    CCMATH mathematics library source code.
  *
  *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
  *  This code may be redistributed under the terms of the GNU library
  *  public license (LGPL). ( See the lgpl.license file for details.)
  * ------------------------------------------------------------------------
  */
 //#include "ccmath.h"
 int psinv(double *v, int n)
 {
     double z, *p, *q, *r, *s, *t;

     int j, k;

     for (j = 0, p = v; j < n; ++j, p += n + 1) {
         for (q = v + j * n; q < p; ++q)
             *p -= *q * *q;
         if (*p <= 0.)
             return -1;
         *p = sqrt(*p);
         for (k = j + 1, q = p + n; k < n; ++k, q += n) {
             for (r = v + j * n, s = v + k * n, z = 0.; r < p;)
                 z += *r++ * *s++;
             *q -= z;
             *q /= *p;
         }
     }
     trnm(v, n);
     for (j = 0, p = v; j < n; ++j, p += n + 1) {
         *p = 1. / *p;
         for (q = v + j, t = v; q < p; t += n + 1, q += n) {
             for (s = q, r = t, z = 0.; s < p; s += n)
                 z -= *s * *r++;
             *q = z * *p;
         }
     }
     for (j = 0, p = v; j < n; ++j, p += n + 1) {
         for (q = v + j, t = p - j; q <= p; q += n) {
             for (k = j, r = p, s = q, z = 0.; k < n; ++k)
                 z += *r++ * *s++;
             *t++ = (*q = z);
         }
     }
     return 0;
 }






 void float_to_fraction(double value, int *numerator, int *denominator, int precision);

double savgolCoef[128];
void SavizkGolayFilter_Coeff(int windSize, int order){
	int halfWindSize = windSize >> 1;
	windSize = (halfWindSize << 1) + 1;	//window size : odd number
	//design matrix
	double D[(order + 1) * windSize];
	for(int i=0; i<windSize; i++){
		double x = (double)i - halfWindSize;	//-half window size <= x <= half window size
		double *pD = &D[i * (order + 1)];
		pD[0] = 1;
		for(int k=1; k<=order; k++){
			pD[k] = pD[k-1] * x;
		}
	}
	//scattering matrix
	double S[(order + 1) * (order + 1)];	//S = ~D.D
	for(int i=0; i<=order; i++){
		for(int j=i; j<=order; j++){
			double s = 0;
			for(int k=0; k<windSize; k++){
				s += D[k * (order + 1) + i] * D[k * (order + 1) + j];
			}
			S[i * (order + 1) + j] = s;
		}
	}
	for(int i=0; i<=order; i++){
		for(int j=0; j<i; j++){
			S[i * (order + 1) + j] = S[j * (order + 1) + i];
		}
	}
	//ccmath library
	psinv(&S[0], order + 1);
	//filter coeffs : 0-th row fo S.~D
	//create filter
	for(int i=0; i<windSize; i++){
		double s = 0;
		for(int k=0; k<=order; k++){
			s += S[k] * D[i * (order + 1) + k];
		}
		savgolCoef[i] = s;
	}
}


#include "stdlib.h"

// 최대공약수를 구하는 함수
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// float 값을 분수로 변환하는 함수
void float_to_fraction(double value, int *numerator, int *denominator, int precision) {
    double integral;
    double frac = modf(value, &integral);
    int sign = (value < 0) ? -1 : 1;

    // 초기 분수 값 설정
    int num = (int)integral;
    int denom = 1;

    // 소수부를 분수로 변환
    for (int i = 0; i < precision; i++) {
        frac *= 10;
        denom *= 10;
    }
    num = num * denom + (int)(frac * sign);

    // 최대공약수를 사용해 분수 간단히 만들기
    int common_divisor = gcd(abs(num), denom);
    *numerator = num / common_divisor;
    *denominator = denom / common_divisor;
}

/**
 *
 */
void SavizkGolayFilter_Merge(int32_t* smooth, int32_t* raw, int srcSize, int order, int windSize){
	static bool init = false;
	if(init == false){
		init = true;
		SavizkGolayFilter_Coeff(windSize, order);
	}

	const int halfWindSize = windSize >> 1;
	windSize = (halfWindSize << 1) + 1;
	int32_t padded[srcSize + windSize];
	memset(padded, 0, sizeof(padded));
	//reflective boundary conditions;
	//[hw]...[1][0][1][2]...[hw]...[n-1+hw][n-2+hw][n-3+hw]...[n-1];

	for(int i=0; i<halfWindSize; i++){
		padded[i] = raw[halfWindSize - i];
		padded[i + srcSize + halfWindSize] = raw[srcSize - 2 - i];
	}
	for(int i = srcSize; i >= 0; i--){
		padded[i + halfWindSize] = raw[i];
	}
	//
	//double smoothed[srcSize];

	for(int i=srcSize; i >= 0; i--){
		int32_t* ppad = &padded[i];
		int fsum = 0;
		for(int k=0; k<windSize; k++){
			fsum += ppad[k] * savgolCoef[k];
		}
		smooth[i] = fsum;
	}
}














