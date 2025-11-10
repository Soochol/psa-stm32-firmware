#ifndef __JH_ES8388_PLATFORM_H
#define __JH_ES8388_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "es8388_driver.h"


typedef enum {
	AUDIO_OUT_LOUT1=0,
	AUDIO_OUT_ROUT1,
	AUDIO_OUT_LOUT2,
	AUDIO_OUT_ROUT2,
	AUDIO_OUT_OUT1,	//LOUT1 + ROUT1
	AUDIO_OUT_OUT2,	//LOUT2 + ROUT2
} e_AUDIO_OUT_t;


#define AUDIO_MONO	0


#define AUDIO_OUTPUT_DISABLE	1	//only test


void v_Codec_RdDone(uint8_t* pu8_arr, uint16_t u16_len);
void v_Codec_WrDone();
void v_Codec_Tout_Handler();

void v_Codec_Deinit();
e_COMM_STAT_t e_Codec_Ready();


void v_AUDIO_Test();
int i_Codec_Volume_Ctrl(int16_t i16_vol);

int i_Codec_Is_Delay();


#ifdef __cplusplus
}
#endif

#endif
