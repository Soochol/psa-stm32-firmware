#include "lib_lpf.h"

#include <stdio.h>

#if 0

double LPF(double U, double ALPHA);	// 저역통과필터 함수
double U_ = 0;	// 1step 이전 입력값
double Y_ = 0;	// 1step 이전 출력값

/*
 * fc : 100 Hz
 *
 */
double LPF(double U, double ALPHA)	// LPF 함수
{
	//double Y = ((1-ALPHA)*Y_ + ALPHA*U + ALPHA*U_)/(1+ALPHA);
	double Y = (Y_ + ALPHA*U)/(1+ALPHA);	// 출력 계산
	U_ = U;	// 1step 이전 데이터 갱신
	Y_ = Y;

	return Y;	// LPF 출력
}

int examples_(void)
{
	double INPUT = 1.0;	// 입력 Ref
	double OUTPUT = 0.0;	// LPF 출력값
	double alpha = 0.06283184;	//= 2*3.141592*100*(1/10000)/2 : Wc * T

	while(1)
	{
		OUTPUT = LPF(INPUT, alpha);	// LPF 출력 값 저장
		printf("%lf\n", OUTPUT);	// LPF 출력값 모니터 출력
		if(OUTPUT > 0.999)	// LPF의 출력이 0.999 이상인 경우
		 break;	// 루프 종료
	}
}



#endif


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




