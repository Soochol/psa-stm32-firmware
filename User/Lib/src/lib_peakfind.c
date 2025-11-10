#include "lib_peakfind.h"





void _v_PeakFind_Next(_x_PEAK_RING_t* px_dst, uint16_t x, PEAK_PRECISION y, _x_PEAK_VALID_t* px_valid);




/**
 * parameter
 * - px_dst		: destination
 * - ps32_src	: source
 * - u16_cnt	: source counts
 *
 * - time_unit
 * - min_range	: validate minimum range
 * - max_range	: validate maximum range
 *
 *
 */
void _v_PeakFind(_x_PEAK_VALID_t* px_valid, _x_PEAK_RING_t* px_dst, PEAK_PRECISION* p_src, uint16_t u16_srcCnt){
	static _x_PEAK_XY_t peak;
	static uint16_t outCnt;
	uint16_t srcIdx = 0;
	//up -> down -> up -> ....
	if(px_dst->arr[0].y == 0){
		PEAK_PRECISION max = 0;

		while(outCnt < u16_srcCnt){
			if(p_src[outCnt] > max){
				max = p_src[outCnt];
				px_dst->arr[0].x = outCnt;
				px_dst->arr[0].y = max;
			}
			outCnt++;
		}
		//zero point
		px_dst->u16_in++;
		px_dst->u16_cnt++;

		outCnt = peak.x + 1;
		srcIdx = outCnt;
		u16_srcCnt -= (outCnt + 1);
	}
	while(u16_srcCnt--){
		_v_PeakFind_Next(px_dst, outCnt++, p_src[srcIdx++], px_valid);

		//_b_SSF_PeakDetect(p_src, u16_srcCnt, ssf_sum, ssf_peak);
	}
}


/**
 * top :
 * direction : default is 0 -> up
 * Bottom -> Top
 */
void _v_PeakFind_Next(_x_PEAK_RING_t* px_dst, uint16_t x, PEAK_PRECISION y, _x_PEAK_VALID_t* px_valid){
	static PEAK_PRECISION tmpTopY, tmpBotY;
	static PEAK_PRECISION tmpTopX, tmpBotX;
	static PEAK_PRECISION tmpUp, tmpDown;

	static bool overShoot;
	//static bool topSkip, botSkip;
	/*
	px_peak->u16_cnt = odd	//to down
	px_peak->u16_cnt = even	//to up
	*/
	//px_peak	: max
	if(px_dst->u16_in & 1){
		//to down
		if((tmpDown == 0) || (tmpDown >= y)){
			tmpDown = y;
		}
		else{
			tmpBotY = tmpDown;
			tmpBotX = x - 1;
			//check Top
			uint16_t itv;

			_x_PEAK_XY_t currTop = px_dst->arr[(px_dst->u16_in - 1) & px_dst->u16_mask];
			_x_PEAK_XY_t prevBot = px_dst->arr[(px_dst->u16_in - 2) & px_dst->u16_mask];
			_x_PEAK_XY_t prevTop = px_dst->arr[(px_dst->u16_in - 3) & px_dst->u16_mask];

			int32_t prevThld = prevTop.y - prevBot.y;
			int32_t currThld = currTop.y - tmpBotY;
			bool thld = false;

			if(tmpBotX > prevBot.x){
				itv = tmpBotX - prevBot.x;
			}
			else{
				itv = (0xFFFF - prevBot.x) + tmpBotX;
			}

			if(currThld > (prevThld * 0.5)){
				thld = true;
			}

			if(overShoot != false){
				overShoot = false;
				itv = px_valid->u16_minX;
				thld = true;
			}

			if((itv >= px_valid->u16_minX) && (itv <= px_valid->u16_maxX) && (thld)){
				//valid
				px_dst->arr[px_dst->u16_in].x = tmpBotX;
				px_dst->arr[px_dst->u16_in].y = tmpBotY;
				px_dst->u16_in = (px_dst->u16_in + 1) & px_dst->u16_mask;
				if(px_dst->u16_cnt <= px_dst->u16_mask){
					px_dst->u16_cnt++;
				}
				else{
					px_dst->u16_out = (px_dst->u16_out + 1) & px_dst->u16_mask;
				}
				//tmpBotX = 0;
				tmpDown = 0;
			}
			else{
				//invalid
				if(itv > px_valid->u16_maxX){
					//default
					tmpDown = 0;
					overShoot = true;
				}
				else{
					//skip
					//botSkip = true;
					//tmpDown = 0;
				}
			}
		}
	}
	else{
		//to up
		if((tmpUp == 0) || (tmpUp <= y)){
			tmpUp = y;
		}
		else{
			tmpTopY = tmpUp;
			tmpTopX = x - 1;
			//check Bottom

			uint16_t itv;

			_x_PEAK_XY_t currBot = px_dst->arr[(px_dst->u16_in - 1) & px_dst->u16_mask];
			_x_PEAK_XY_t prevTop = px_dst->arr[(px_dst->u16_in - 2) & px_dst->u16_mask];
			_x_PEAK_XY_t prevBot = px_dst->arr[(px_dst->u16_in - 3) & px_dst->u16_mask];

			bool thld = false;
			int32_t prevThld = prevTop.y - prevBot.y;
			int32_t currThld = tmpTopY - currBot.y;

			if(currThld > (prevThld * 0.5)){
				thld = true;
			}

			if(tmpTopX > prevTop.x){
				itv = tmpTopX - prevTop.x;
			}
			else{
				itv = (0xFFFF - prevTop.x) + tmpTopX;
			}

			//previous value default... (idx - 2)
			if(overShoot != false){
				overShoot = false;
				thld = true;
				itv = px_valid->u16_minX;
			}


			//if time average is different.
			//example data is trust?


			if((itv >= px_valid->u16_minX) && (itv <= px_valid->u16_maxX) && (thld)){
				//valid
				px_dst->arr[px_dst->u16_in].x = tmpTopX;
				px_dst->arr[px_dst->u16_in].y = tmpTopY;
				px_dst->u16_in = (px_dst->u16_in + 1) & px_dst->u16_mask;
				if(px_dst->u16_cnt <= px_dst->u16_mask){
					px_dst->u16_cnt++;
				}
				else{
					px_dst->u16_out = (px_dst->u16_out + 1) & px_dst->u16_mask;
				}
				tmpUp = 0;
				//
				//tmpBotX = 0;
			}
			else{
				//invalid
				if(itv > px_valid->u16_maxX){
					//default
					tmpUp = 0;
					overShoot = true;
				}
				else{
					//skip
					//topSkip = true;
					//tmpUp = 0;
				}
			}
		}
	}
}




	static PEAK_PRECISION slopeSum[256];
bool _b_SSF_PeakDetect(PEAK_PRECISION* rawArr, uint16_t u16_ArrCnt, PEAK_PRECISION* peakArr, uint16_t* pu16_peakCnt){
	static uint8_t in=1, out=1, cnt=0;

	cnt += u16_ArrCnt;
	if(cnt < 2){return false;}
	//slope sum
	PEAK_PRECISION thld = 1;

	for(uint16_t i=1; i<cnt; i++, in++){
		slopeSum[in] = slopeSum[(uint8_t)(in - 1)] + rawArr[i] - rawArr[i - 1];
	}

	uint16_t len = cnt;
	for(uint16_t i=1; i<len-1; i++, cnt--){
		if(slopeSum[out] > thld && \
		   slopeSum[out] > slopeSum[(uint8_t)(out - 1)] && \
		   slopeSum[out] > slopeSum[(uint8_t)(out + 1)]){
			*peakArr++ = i - 1;
			(*pu16_peakCnt)++;
		}
	}
	return true;
}






























