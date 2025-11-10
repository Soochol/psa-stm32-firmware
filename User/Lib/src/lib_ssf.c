#include <lib_ssf.h>
#include <string.h>


/**
 * data type : ???????
 *
 */

static void _v_SSF_SlopeSum(SSF_PRECISION* raw, uint16_t u16_rawSize, SSF_PRECISION* slope, SSF_PRECISION* slopeSum,uint16_t u16_windSize){
	uint16_t i;
	for(i=0; i<u16_rawSize-1; i++){
		slope[i] = raw[i + 1] - raw[i];
	}

	//sliding window sum
	for(i=0; i<u16_rawSize - u16_windSize; i++){
		SSF_PRECISION sum = 0;
		for(int j=0; j<u16_windSize; j++){
			sum += slope[i + j];
		}
		slopeSum[i] = sum;
	}
}


/**
 * return
 * - peakCnt	: it's detected peak counts.
 *
 */
uint16_t _u16_SSF_Handler(SSF_PRECISION* raw, uint16_t u16_rawSize, uint16_t u16_windSize, uint16_t* pu16_peakPos, uint16_t u16_posSize){
	SSF_PRECISION slope[u16_rawSize];	//region variable.. not used ring buffer.
	SSF_PRECISION slopeSum[u16_rawSize];

	SSF_PRECISION sumNeg = 0;
	SSF_PRECISION rawCpy[u16_rawSize];
	memcpy(rawCpy, raw, sizeof(SSF_PRECISION)*u16_rawSize);
	//slope sum
	_v_SSF_SlopeSum(raw, u16_rawSize, slope, slopeSum, u16_windSize);
	//peak detect
	uint16_t peakCnt=0;
	for(uint16_t i=0; i<u16_rawSize - u16_windSize - 1; i++){
		//slopeSum > 0 : to up
		//slopeSum[i] > slopeSum[i+1] : up -> down or bending
		//slope[i + wind - 1] > 0	: last index check that it's to up
		//if(slopeSum[i] > 0 && slopeSum[i] > slopeSum[i + 1] && slope[i + u16_windSize - 1] < 0){
		if(slope[i] > 0 && slopeSum[i] > slopeSum[i + 1] && slope[i + u16_windSize - 1] < 0){
			//rate
			uint16_t positive=0, negative=0;
			uint16_t turnIdx=0;
			SSF_PRECISION max=0;
			for(uint16_t j=0; j<u16_windSize; j++){
				if(slope[j] > 0){
					positive++;
					negative=0;
					if(max < raw[j]){
						max = raw[j];
						turnIdx = j;
					}
					sumNeg = 0;
				}
				else if(slope[j] < 0){
					negative++;
					sumNeg += slope[j];
				}
				else{
					//sumNeg += 1;	//penalty	//in sampling speed increase, issue..
				}
			}

			if(positive && slopeSum[i] < -30){
			//if(positive && negative > 4 && sumNeg < -10){

				peakCnt++;
				if(u16_posSize--){
					*pu16_peakPos++ = turnIdx;
				}
			}
		}
	}
	return peakCnt;
}


/*
 * brief	: slope sum function handler
 * date
 * create	: 25.03.20
 * modify	: -
 */
uint16_t _u16_SSF_HandlerT(SSF_PRECISION* raw, SSF_PRECISION* pattern, uint16_t u16_rawSize, uint16_t u16_windSize, uint16_t* pu16_peakPos, uint16_t u16_posSize){
	SSF_PRECISION slope[u16_rawSize];	//region variable.. not used ring buffer.
	SSF_PRECISION slopeSum[u16_rawSize];

	SSF_PRECISION sumNeg = 0;
	SSF_PRECISION rawCpy[u16_rawSize];
	memcpy(rawCpy, raw, sizeof(SSF_PRECISION)*u16_rawSize);
	//slope sum
	_v_SSF_SlopeSum(raw, u16_rawSize, slope, slopeSum, u16_windSize);
	//peak detect
	uint16_t peakCnt=0;
	for(uint16_t i=0; i<u16_rawSize - u16_windSize - 1; i++){
		//slopeSum > 0 : to up
		//slopeSum[i] > slopeSum[i+1] : up -> down or bending
		//slope[i + wind - 1] > 0	: last index check that it's to up
		//if(slopeSum[i] > 0 && slopeSum[i] > slopeSum[i + 1] && slope[i + u16_windSize - 1] < 0){
		if(slope[i] > 0 && slopeSum[i] > slopeSum[i + 1] && slope[i + u16_windSize - 1] < 0){
			//rate
			uint16_t positive=0, negative=0;
			uint16_t turnIdx=0;
			SSF_PRECISION max=0;
			for(uint16_t j=0; j<u16_windSize; j++){
				if(slope[j] > 0){
					positive++;
					negative=0;
					if(max < raw[j]){
						max = raw[j];
						turnIdx = j;
					}
					sumNeg = 0;
				}
				else if(slope[j] < 0){
					negative++;
					sumNeg += slope[j];
				}
				else{
					//sumNeg += 1;	//penalty	//in sampling speed increase, issue..
				}
			}

			if(positive && slopeSum[i] < -30){
			//if(positive && negative > 4 && sumNeg < -10){

				peakCnt++;
				if(u16_posSize--){
					*pu16_peakPos++ = turnIdx;
				}
			}
		}
	}
	return peakCnt;
}






