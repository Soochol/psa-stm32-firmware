#include "main.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3_platform.h"
#include "es8388_platform.h"
#include "uart.h"
#include "stdio.h"
#include <math.h>
#include "tim.h"
#include "sd.h"
#include "fatfs.h"



extern SAI_HandleTypeDef hsai_BlockA1;
extern SAI_HandleTypeDef hsai_BlockB1;

extern DMA_HandleTypeDef hdma_sai1_a;
extern DMA_HandleTypeDef hdma_sai1_b;

SAI_HandleTypeDef* p_A1 = &hsai_BlockA1;
SAI_HandleTypeDef* p_B1 = &hsai_BlockB1;

#define MP3_MONO	1
#if MP3_MONO
#define PLAY_BUF_SAMPLES	2304	//2304	//576
//#define PLAY_BUF_SAMPLES	1152	//2304	//576
#else
#define PLAY_BUF_SAMPLES	4608
#endif
#define MP3_FILE_SIZE	(4096)

//MINIMP3
static mp3dec_t mp3dec;
static mp3dec_frame_info_t mp3_info;
static mp3d_sample_t mp3_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME + 1];
#if MP3_MONO
static mp3d_sample_t mp3_pcm_mono[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
#endif
ALIGN_32BYTES(uint8_t mp3_file[MP3_FILE_SIZE]);
//FATFS
static FIL mp3File;








//test
uint32_t mp3ReadCnt;
uint32_t hal_mp3_ok, hal_mp3_busy, hal_mp3_err;
uint16_t etc_cnt;
uint16_t mp3_num;



//////////////////////////
//			SAI			//
//////////////////////////
ALIGN_32BYTES(uint32_t dummy[4] __attribute__((section(".my_nocache_section"))));
ALIGN_32BYTES(int16_t  sai_buf[PLAY_BUF_SAMPLES] __attribute__((section(".my_nocache_section"))));
volatile uint16_t buf_state;

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai){
	if(hsai == p_B1){
		buf_state = 1;
	}
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai){
	if(hsai == p_B1){
		buf_state = 2;
	}
}

/** WAV → SAI PCM 채널 재조정 함수(16-bit, stereo 고정) */
static void feed_buffer(uint32_t offset_words, int16_t *pcm, int samples, int ch)
{
    /* sai_buf 는 int16_t L,R 인터리브 */
    memcpy(&sai_buf[offset_words], pcm, samples * ch * sizeof(int16_t));
    //0 or half
}


//////////////////////////
//			DAC			//
//////////////////////////
extern TIM_HandleTypeDef htim1;
static TIM_HandleTypeDef* p_tim = &htim1;

extern DAC_HandleTypeDef hdac1;
static DAC_HandleTypeDef* p_dac = &hdac1;

ALIGN_32BYTES(uint16_t  dac_buf[PLAY_BUF_SAMPLES] __attribute__((section(".my_nocache_section"))));
volatile uint16_t dac_buf_state;

uint16_t dac_vol = 50;


#define VOLUME_ZERO_DB	60.0f	//-60
#define VOLUME_THLD		5
uint16_t volumeTable[101];

void Volume_table_Create(){
	for(int i=0; i<=100; i++){
		float db = -60.0f + (60.0f * i / 100.0f);	//-90dB ~ 0dB
		float linear = pow(10.0f, db / 20.0f);		//10^(dB / 20)
		volumeTable[i] = (uint16_t)(linear * 65535.0f);
	}
}

void fill_dac_buffer_mono(uint32_t offset, int16_t* pcm, int samples){
	uint16_t* p = dac_buf + offset;
	int32_t scaled;
	for(int i=0; i<samples; i++){
		if(dac_vol < VOLUME_THLD){
			p[i] = 2048;
		}
		else{
			scaled = (int32_t)pcm[i] * volumeTable[dac_vol];
			scaled >>= 16;
			p[i] = (scaled + 32768) >> 4;
		}
	}
}

void fill_dac_buffer_stereo(uint32_t offset, int16_t* pcm, int samples){
	uint16_t* p = dac_buf + offset;
	for(int i=0; i<samples; i++){
		//OUT1
		p[(i<<1)] = pcm[i] + 32768;	//convert to unsigned
		p[(i<<1) + 1] = 0;			//unused
	}
}

void fill_dac_buffer(uint32_t offset, int16_t* pcm, int samples, int ch){
	if(ch == 2){
		fill_dac_buffer_stereo(offset, pcm, samples);
	}
	else{
		fill_dac_buffer_mono(offset, pcm, samples);
	}
}


void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac){
	dac_buf_state = 1;
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac){
	dac_buf_state = 2;
}













const char* mp3_list[] = {
		"01.mp3",
		"02.mp3",
		"03.mp3",
		"04.mp3",
		"05.mp3",
		"06.mp3",
		"07.mp3",
		"08.mp3",
		"09.mp3",
		"10.mp3",
		"11.mp3",
		"12.mp3",
		"13.mp3",
		"14.mp3",
		"15.mp3",
		"16.mp3",
		"17.mp3",
		"18.mp3",
		"19.mp3",
		"20.mp3",
		"21.mp3",
		"22.mp3",
		"23.mp3",
		"24.mp3",
		"25.mp3",
		"26.mp3",
		"27.mp3",
		"28.mp3",
		"29.mp3",
		"30.mp3",
		"31.mp3",
		"32.mp3",
		"33.mp3",
};

void v_MP3_Init(){
	mp3dec_init(&mp3dec);
}

void v_MP3_Play_T(){
	static bool clock_alive;
	bool mp3_end = false;
	if(clock_alive == false){
		clock_alive = true;

		memset(dummy, 0, sizeof(dummy));
		HAL_SAI_Transmit_DMA(p_A1, (uint8_t*)dummy, sizeof(dummy) / 2);
	}
	//	SD	//
	size_t mp3_offset = 0;
	UINT byteRead;
	UINT remain;

	if(b_IsMountSD() == false){
		printf("sd mount fail.\n");
		return;
	}
	else{
		printf("sd mount succ.\n");
	}

	if(mp3_num >= 33){mp3_num = 0;}
	FRESULT retFile = f_open(&mp3File, mp3_list[mp3_num++], FA_READ);

	if(retFile != FR_OK){
		printf("sd file open fail.\n");
		return;
	}
	else{
		printf("sd file open succ.\n");
	}

	retFile = f_read(&mp3File, mp3_file, MP3_FILE_SIZE, &byteRead);
	remain = byteRead;	//4096

	if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
	else						{mp3_end = true;}
	int samples = mp3dec_decode_frame(&mp3dec, mp3_file, remain, mp3_pcm, &mp3_info);
	mp3_offset += mp3_info.frame_bytes;
	remain -= mp3_info.frame_bytes;

	//mono : left (0) / right (used)
	for(size_t i = 0; i < samples; i++){
		mp3_pcm_mono[2*i] = 0;
		mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
	}

	//circular
	HAL_StatusTypeDef ret = HAL_SAI_Transmit_DMA(p_B1, (uint8_t*)sai_buf, PLAY_BUF_SAMPLES);
	if(ret != HAL_OK){
		if(ret == HAL_BUSY)	{hal_mp3_busy++;}
		else				{hal_mp3_err++;}
	}
	else					{hal_mp3_ok++;}

	feed_buffer(PLAY_BUF_SAMPLES / 2, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0

	while(!mp3_end){
		if(buf_state != 0){
			uint32_t dst = (buf_state==1) ? 0 : PLAY_BUF_SAMPLES / 2;
			buf_state = 0;

			if(mp3_end && remain == 0){break;}
			if(remain < 512){	//512 -> 384 (
				memmove(mp3_file, mp3_file + mp3_offset, remain);
				retFile = f_read(&mp3File, &mp3_file[remain], (MP3_FILE_SIZE - remain), &byteRead);
				remain += byteRead;
				mp3_offset = 0;

				if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
				else{
					mp3_end = true;
				}
			}

			if(remain){
				int samples = mp3dec_decode_frame(&mp3dec, &mp3_file[mp3_offset], remain, mp3_pcm, &mp3_info);
				mp3ReadCnt++;
				mp3_offset += mp3_info.frame_bytes;
				remain -= mp3_info.frame_bytes;

				//mono : left (0) / right (used)
				for(size_t i = 0; i < samples; i++){
					mp3_pcm_mono[2*i] = 0;
					mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
				}

				if(samples){
					feed_buffer(dst, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
				}
				else{
					if(mp3_info.frame_bytes == 0){
						memset(&sai_buf[dst], 0, (PLAY_BUF_SAMPLES/2)*2);
						break;
					}
					else{
						//frame byte > 0 && sample == 0
						etc_cnt++;
					}
				}
			}
		}
	}
	clock_alive = false;
	HAL_DMA_Abort(p_A1->hdmatx);
	f_close(&mp3File);

	mp3ReadCnt = 0;
	etc_cnt = 0;
}




void v_MP3_Test(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 5000)){
		timRef = u32_Tim_1msGet();
		HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);
		v_MP3_Play_T();
		HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);
		//v_MP3_Play();
	}
}


static bool b_file_open;



int i_MP3_Play(uint16_t u16_mp3_num){
	static bool clock_alive;
	bool mp3_end = false;
	if(clock_alive == false){
		clock_alive = true;

		memset(dummy, 0, sizeof(dummy));
		HAL_SAI_Transmit_DMA(p_A1, (uint8_t*)dummy, sizeof(dummy) / 2);
	}
	//	SD	//
	size_t mp3_offset = 0;
	UINT byteRead;
	UINT remain;

	if(b_IsMountSD() == false){
		printf("sd mount fail.\n");
		return - 1;
	}
	else{
		printf("sd mount succ.\n");
	}

	if(u16_mp3_num >= 33){return false;}
	FRESULT retFile = f_open(&mp3File, mp3_list[mp3_num++], FA_READ);
	if(retFile != FR_OK){
		b_file_open = false;
		printf("sd file open fail.\n");
		return -1;
	}
	else{
		b_file_open = true;
		printf("sd file open succ.\n");
	}

	//speaker open
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);

	retFile = f_read(&mp3File, mp3_file, MP3_FILE_SIZE, &byteRead);
	remain = byteRead;	//4096

	if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
	else						{mp3_end = true;}
	int samples = mp3dec_decode_frame(&mp3dec, mp3_file, remain, mp3_pcm, &mp3_info);
	mp3_offset += mp3_info.frame_bytes;
	remain -= mp3_info.frame_bytes;

	//mono : left (0) / right (used)
	for(size_t i = 0; i < samples; i++){
		mp3_pcm_mono[2*i] = 0;
		mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
	}

	//circular
	HAL_StatusTypeDef ret = HAL_SAI_Transmit_DMA(p_B1, (uint8_t*)sai_buf, PLAY_BUF_SAMPLES);
	if(ret != HAL_OK){
		if(ret == HAL_BUSY)	{hal_mp3_busy++;}
		else				{hal_mp3_err++;}
	}
	else					{hal_mp3_ok++;}

	feed_buffer(PLAY_BUF_SAMPLES / 2, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0

	while(!mp3_end){
		if(buf_state != 0){
			uint32_t dst = (buf_state==1) ? 0 : PLAY_BUF_SAMPLES / 2;
			buf_state = 0;

			if(mp3_end && remain == 0){break;}
			if(remain < 512){	//512 -> 384 (
				memmove(mp3_file, mp3_file + mp3_offset, remain);
				retFile = f_read(&mp3File, &mp3_file[remain], (MP3_FILE_SIZE - remain), &byteRead);	//SDMMC
				remain += byteRead;
				mp3_offset = 0;

				if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
				else{
					mp3_end = true;
				}
			}

			if(remain){
				int samples = mp3dec_decode_frame(&mp3dec, &mp3_file[mp3_offset], remain, mp3_pcm, &mp3_info);
				mp3ReadCnt++;
				mp3_offset += mp3_info.frame_bytes;
				remain -= mp3_info.frame_bytes;

				//mono : left (0) / right (used)
				for(size_t i = 0; i < samples; i++){
					mp3_pcm_mono[2*i] = 0;
					mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
				}

				if(samples){
					feed_buffer(dst, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
				}
				else{
					if(mp3_info.frame_bytes == 0){
						memset(&sai_buf[dst], 0, (PLAY_BUF_SAMPLES/2)*2);
						break;
					}
					else{
						//frame byte > 0 && sample == 0
						etc_cnt++;
					}
				}
			}
		}
	}

	//speaker close
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);

	clock_alive = false;
	HAL_DMA_Abort(p_A1->hdmatx);
	f_close(&mp3File);
	b_file_open = false;

	mp3ReadCnt = 0;
	etc_cnt = 0;

	return 0;
}



float mp3_time;

int i_MP3_Play_T(uint16_t u16_mp3_num){
	static bool clock_alive;
	bool mp3_end = false;
	if(clock_alive == false){
		clock_alive = true;

		memset(dummy, 0, sizeof(dummy));
		HAL_SAI_Transmit_DMA(p_A1, (uint8_t*)dummy, sizeof(dummy) / 2);
	}
	//	SD	//
	size_t mp3_offset = 0;
	UINT byteRead;
	UINT remain;

	if(b_IsMountSD() == false){
		printf("sd mount fail.\n");
		return - 1;
	}
	else{
		printf("sd mount succ.\n");
	}

	u16_mp3_num--;
	if(u16_mp3_num >= 33){return false;}
	FRESULT retFile = f_open(&mp3File, mp3_list[u16_mp3_num], FA_READ);
	if(retFile != FR_OK){
		b_file_open = false;
		printf("sd file open fail.\n");
		return -1;
	}
	else{
		b_file_open = true;
		printf("sd file open succ.\n");
	}

	//timer start
	uint32_t start = DWT->CYCCNT;

	//speaker open
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);

	retFile = f_read(&mp3File, mp3_file, MP3_FILE_SIZE, &byteRead);
	remain = byteRead;	//4096

	if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
	else						{mp3_end = true;}
	int samples = mp3dec_decode_frame(&mp3dec, mp3_file, remain, mp3_pcm, &mp3_info);
	mp3_offset += mp3_info.frame_bytes;
	remain -= mp3_info.frame_bytes;

	//mono : left (0) / right (used)
	for(size_t i = 0; i < samples; i++){
		mp3_pcm_mono[2*i] = 0;
		mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
	}

	//circular
	HAL_StatusTypeDef ret = HAL_SAI_Transmit_DMA(p_B1, (uint8_t*)sai_buf, PLAY_BUF_SAMPLES);
	if(ret != HAL_OK){
		if(ret == HAL_BUSY)	{hal_mp3_busy++;}
		else				{hal_mp3_err++;}
	}
	else					{hal_mp3_ok++;}

	feed_buffer(PLAY_BUF_SAMPLES / 2, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
	while(!mp3_end){
		if(buf_state != 0){
			uint32_t dst = (buf_state==1) ? 0 : PLAY_BUF_SAMPLES / 2;
			buf_state = 0;

			if(mp3_end && remain == 0){break;}
			if(remain < 512){	//512 -> 384 (
				memmove(mp3_file, mp3_file + mp3_offset, remain);
				retFile = f_read(&mp3File, &mp3_file[remain], (MP3_FILE_SIZE - remain), &byteRead);	//SDMMC
				remain += byteRead;
				mp3_offset = 0;

				if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
				else{
					mp3_end = true;
				}
			}

			if(remain){
				int samples = mp3dec_decode_frame(&mp3dec, &mp3_file[mp3_offset], remain, mp3_pcm, &mp3_info);
				mp3ReadCnt++;
				mp3_offset += mp3_info.frame_bytes;
				remain -= mp3_info.frame_bytes;

				//mono : left (0) / right (used)
				for(size_t i = 0; i < samples; i++){
					mp3_pcm_mono[2*i] = 0;
					mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
				}

				if(samples){
					feed_buffer(dst, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
				}
				else{
					if(mp3_info.frame_bytes == 0){
						memset(&sai_buf[dst], 0, (PLAY_BUF_SAMPLES/2)*2);
						break;
					}
					else{
						//frame byte > 0 && sample == 0
						etc_cnt++;
					}
				}
			}
		}
	}
	//speaker close
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);

	clock_alive = false;
	HAL_DMA_Abort(p_A1->hdmatx);

	uint32_t stop = DWT->CYCCNT;
	uint32_t itv;
	if(stop >= start){itv = stop - start;}
	else{itv = 0xFFFFFFFF - start + stop;}
	mp3_time = itv / 384000000.0;

	f_close(&mp3File);
	b_file_open = false;

	mp3ReadCnt = 0;
	etc_cnt = 0;

	return 0;
}




#define MP3_DAC_SPK_OPEN	1
/*
 * brief	: decode mp3 output to  DAC
 */
int i_MP3_Play_DAC(uint16_t u16_mp3_num){
	bool mp3_end = false;

	//	SD	//
	size_t mp3_offset = 0;
	UINT byteRead;
	UINT remain;

	if(b_IsMountSD() == false){
		printf("sd mount fail.\n");
		return - 1;
	}
	else{
		printf("sd mount succ.\n");
	}

	u16_mp3_num--;
	if(u16_mp3_num >= 33){return false;}
	FRESULT retFile = f_open(&mp3File, mp3_list[mp3_num], FA_READ);
	if(retFile != FR_OK){
		b_file_open = false;
		printf("sd file open fail.\n");
		return -1;
	}
	else{
		b_file_open = true;
		printf("sd file open succ.\n");
	}

	//timer start
	uint32_t start = DWT->CYCCNT;

#if MP3_DAC_SPK_OPEN
	//speaker open
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);
#endif
	retFile = f_read(&mp3File, mp3_file, MP3_FILE_SIZE, &byteRead);
	remain = byteRead;	//4096

	if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
	else						{mp3_end = true;}
	int samples = mp3dec_decode_frame(&mp3dec, mp3_file, remain, mp3_pcm, &mp3_info);
	mp3_offset += mp3_info.frame_bytes;
	remain -= mp3_info.frame_bytes;

	HAL_TIM_Base_Start(p_tim);
	HAL_DAC_Start_DMA(p_dac, DAC_CHANNEL_1, (uint32_t*)dac_buf, PLAY_BUF_SAMPLES, DAC_ALIGN_12B_R);


	fill_dac_buffer(PLAY_BUF_SAMPLES / 2, mp3_pcm, samples, mp3_info.channels);

	while(!mp3_end){
		if(dac_buf_state != 0){
			uint32_t dst = (dac_buf_state==1) ? 0 : PLAY_BUF_SAMPLES / 2;
			dac_buf_state = 0;

			if(mp3_end && remain == 0){break;}
			if(remain < 512){	//512 -> 384 (
				memmove(mp3_file, mp3_file + mp3_offset, remain);
				retFile = f_read(&mp3File, &mp3_file[remain], (MP3_FILE_SIZE - remain), &byteRead);	//SDMMC
				remain += byteRead;
				mp3_offset = 0;

				if(remain == MP3_FILE_SIZE)	{mp3_end = false;}
				else{
					mp3_end = true;
				}
			}

			if(remain){
				int samples = mp3dec_decode_frame(&mp3dec, &mp3_file[mp3_offset], remain, mp3_pcm, &mp3_info);
				mp3ReadCnt++;
				mp3_offset += mp3_info.frame_bytes;
				remain -= mp3_info.frame_bytes;

				if(samples){
					fill_dac_buffer(dst, mp3_pcm, samples, mp3_info.channels);
				}
				else{
					if(mp3_info.frame_bytes == 0){
						memset(&dac_buf[dst], 0, (PLAY_BUF_SAMPLES/2)*2);
						break;
					}
					else{
						//frame byte > 0 && sample == 0
						etc_cnt++;
					}
				}
			}
		}
	}
	//speaker close
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);

	HAL_TIM_Base_Stop(p_tim);
	HAL_DACEx_DualStop_DMA(p_dac, DAC_CHANNEL_1);
	//timer stop
	uint32_t stop = DWT->CYCCNT;
	uint32_t itv;
	if(stop >= start){itv = stop - start;}
	else{itv = 0xFFFFFFFF - start + stop;}
	mp3_time = itv / 384000000.0;

	f_close(&mp3File);
	b_file_open = false;

	mp3ReadCnt = 0;
	etc_cnt = 0;

	return 0;
}

void v_MP3_DAC_Test(){
	static uint32_t timRef, timItv;
	static uint16_t mp3=1;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){

		//if(i_MP3_Play_T(1) == 0){
		if(i_MP3_Play_DAC(1) == 0){
			if(mp3 < 33){mp3++;}
			else		{mp3 = 1;}

			timRef = u32_Tim_1msGet();
			timItv = 5000;
		}
	}
}


///////////////////////////////////
//#define MP3_LOG_ENABLED	1
#define MP3_ADD_DELAY	1

static size_t mp3_offset;
static bool mp3_end;
static UINT mp3_remain;

#if MP3_ADD_DELAY

static int i_spk_open;
static uint32_t u32_spk_timRef;
int i_MP3_Start(uint16_t u16_mp3_num){
	memset(dummy, 0, sizeof(dummy));
	memset(sai_buf, 0, sizeof(sai_buf));
	HAL_SAI_Transmit_DMA(p_A1, (uint8_t*)dummy, sizeof(dummy) / 2);

	//	SD	//
	mp3_offset = 0;
	mp3_end = false;

	if(b_IsMountSD() == false){
		return - 1;
	}
	u16_mp3_num -= 1;
	if(u16_mp3_num >= 33){return false;}
	FRESULT retFile = f_open(&mp3File, mp3_list[u16_mp3_num], FA_READ);
	if(retFile != FR_OK){
		b_file_open = false;
		return -1;
	}
	else{
		b_file_open = true;
	}

	//speaker open
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);
	i_spk_open = 1;
	u32_spk_timRef = u32_Tim_1msGet();
	return 0;
}

int i_MP3_Stop(){
	memset(sai_buf, 0, sizeof(sai_buf));
	//speaker close
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);

	//HAL_DMA_Abort(p_A1->hdmatx);
	HAL_SAI_DMAStop(p_A1);
	HAL_SAI_DMAStop(p_B1);

	__HAL_DMA_CLEAR_FLAG(&hdma_sai1_b, DMA_FLAG_TCIF1_5 | DMA_FLAG_HTIF1_5 | DMA_FLAG_TEIF1_5 | DMA_FLAG_DMEIF1_5);

	if(b_file_open){
		f_close(&mp3File);
		b_file_open = false;
	}
	mp3_end = false;
	return 0;
}



int i_MP3_Decode(){
	if(i_spk_open){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_spk_timRef, 100)){
			//circular
			HAL_SAI_Transmit_DMA(p_B1, (uint8_t*)sai_buf, PLAY_BUF_SAMPLES);
			i_spk_open = 0;
		}
	}
	else{
		if(buf_state != 0){
			uint32_t dst = (buf_state==1) ? 0 : PLAY_BUF_SAMPLES / 2;
			buf_state = 0;
			UINT byteRead;
			//if(mp3_end && mp3_remain == 0){break;}
			if(mp3_remain < 512){	//512 -> 384 (
				memmove(mp3_file, mp3_file + mp3_offset, mp3_remain);
				f_read(&mp3File, &mp3_file[mp3_remain], (MP3_FILE_SIZE - mp3_remain), &byteRead);	//SDMMC
				mp3_remain += byteRead;
				mp3_offset = 0;

				if(mp3_remain == MP3_FILE_SIZE)	{mp3_end = false;}
				else{
					//mp3_end = true;
				}
			}

			if(mp3_remain){
				int samples = mp3dec_decode_frame(&mp3dec, &mp3_file[mp3_offset], mp3_remain, mp3_pcm, &mp3_info);
				mp3ReadCnt++;
				mp3_offset += mp3_info.frame_bytes;
				mp3_remain -= mp3_info.frame_bytes;

				//mono : left (0) / right (used)
				for(size_t i = 0; i < samples; i++){
					mp3_pcm_mono[2*i] = 0;
					mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
				}

				if(samples){
					feed_buffer(dst, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
				}
				else{
					if(mp3_info.frame_bytes == 0){
						memset(&sai_buf[dst], 0, (PLAY_BUF_SAMPLES/2)*2);
						//mp3_end = true;
					}
					else{
						//frame byte > 0 && sample == 0
						etc_cnt++;
						//mp3_end = true;
					}
				}
			}
			else{
				mp3_end = true;
			}
		}
	}
	return mp3_end;
}

#else

int i_MP3_Start(uint16_t u16_mp3_num){
	memset(dummy, 0, sizeof(dummy));
	HAL_SAI_Transmit_DMA(p_A1, (uint8_t*)dummy, sizeof(dummy) / 2);

	//	SD	//
	mp3_offset = 0;
	mp3_end = false;
	UINT byteRead;

	if(b_IsMountSD() == false){
		return - 1;
	}
	u16_mp3_num -= 1;
	if(u16_mp3_num >= 33){return false;}
	FRESULT retFile = f_open(&mp3File, mp3_list[u16_mp3_num], FA_READ);
	if(retFile != FR_OK){
		b_file_open = false;
		return -1;
	}
	else{
		b_file_open = true;
	}

	//speaker open
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);


	retFile = f_read(&mp3File, mp3_file, MP3_FILE_SIZE, &byteRead);
	mp3_remain = byteRead;	//4096

	if(mp3_remain == MP3_FILE_SIZE)	{mp3_end = false;}
	else							{mp3_end = true;}
	int samples = mp3dec_decode_frame(&mp3dec, mp3_file, mp3_remain, mp3_pcm, &mp3_info);
	mp3_offset += mp3_info.frame_bytes;
	mp3_remain -= mp3_info.frame_bytes;

	//mono : left (0) / right (used)
	for(size_t i = 0; i < samples; i++){
		mp3_pcm_mono[2*i] = 0;
		mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
	}

	//circular
	HAL_StatusTypeDef ret = HAL_SAI_Transmit_DMA(p_B1, (uint8_t*)sai_buf, PLAY_BUF_SAMPLES);
	if(ret != HAL_OK){
		if(ret == HAL_BUSY)	{hal_mp3_busy++;}
		else				{hal_mp3_err++;}
	}
	else					{hal_mp3_ok++;}

	feed_buffer(PLAY_BUF_SAMPLES / 2, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
	return 0;
}

int i_MP3_Stop(){
	//speaker close
	HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);

	//HAL_DMA_Abort(p_A1->hdmatx);
	HAL_SAI_DMAStop(p_A1);
	HAL_SAI_DMAStop(p_B1);
	if(b_file_open){
		f_close(&mp3File);
		b_file_open = false;
	}
	mp3_end = false;
	return 0;
}



int i_MP3_Decode(){
	if(buf_state != 0){
		uint32_t dst = (buf_state==1) ? 0 : PLAY_BUF_SAMPLES / 2;
		buf_state = 0;
		UINT byteRead;
		//if(mp3_end && mp3_remain == 0){break;}
		if(mp3_remain < 512){	//512 -> 384 (
			memmove(mp3_file, mp3_file + mp3_offset, mp3_remain);
			f_read(&mp3File, &mp3_file[mp3_remain], (MP3_FILE_SIZE - mp3_remain), &byteRead);	//SDMMC
			mp3_remain += byteRead;
			mp3_offset = 0;

			if(mp3_remain == MP3_FILE_SIZE)	{mp3_end = false;}
			else{
				//mp3_end = true;
			}
		}

		if(mp3_remain){
			int samples = mp3dec_decode_frame(&mp3dec, &mp3_file[mp3_offset], mp3_remain, mp3_pcm, &mp3_info);
			mp3ReadCnt++;
			mp3_offset += mp3_info.frame_bytes;
			mp3_remain -= mp3_info.frame_bytes;

			//mono : left (0) / right (used)
			for(size_t i = 0; i < samples; i++){
				mp3_pcm_mono[2*i] = 0;
				mp3_pcm_mono[2*i + 1] = mp3_pcm[i];
			}

			if(samples){
				feed_buffer(dst, mp3_pcm_mono, samples, mp3_info.channels * 2);          // Half 0
			}
			else{
				if(mp3_info.frame_bytes == 0){
					memset(&sai_buf[dst], 0, (PLAY_BUF_SAMPLES/2)*2);
					mp3_end = true;
				}
				else{
					//frame byte > 0 && sample == 0
					etc_cnt++;
					mp3_end = true;
				}
			}
		}
	}
	return mp3_end;
}

#endif

//#define MP3_BYPASS

static e_MP3_STAT_t e_mp3_stat;

int i_MP3_Player(uint16_t u16_num){
#ifdef MP3_BYPASS
	e_mp3_stat = MP3_DONE;
#else
	if(e_mp3_stat == MP3_IDLE || e_mp3_stat == MP3_DONE){
		if(i_MP3_Start(u16_num) == 0){
			e_mp3_stat = MP3_START;
		}
		else{
			e_mp3_stat = MP3_ERR;
		}
	}
	if(i_MP3_Decode()){
		i_MP3_Stop();
		e_mp3_stat = MP3_DONE;
	}
	else{
		e_mp3_stat = MP3_BUSY;
	}
#endif
	return e_mp3_stat;
}

int i_MP3_ForceStop(){
	i_MP3_Stop();
	e_mp3_stat = MP3_IDLE;
	return e_mp3_stat;
}

int i_MP3_Is_Ready(){
	if(e_mp3_stat == MP3_IDLE || e_mp3_stat == MP3_DONE){
		return 1;
	}
	else{
		return 0;
	}
}

int i_MP3_Get_Stat(){
	return e_mp3_stat;
}

int i_MP3_Playing(){
#ifdef MP3_BYPASS
	e_mp3_stat = MP3_DONE;
#else
	if(e_mp3_stat == MP3_BUSY || e_mp3_stat == MP3_START){
		if(i_MP3_Decode()){
			i_MP3_Stop();
			e_mp3_stat = MP3_DONE;
		}
		else{
			e_mp3_stat = MP3_BUSY;
		}
	}
#endif
	return e_mp3_stat;
}

int i_MP3_Begin(uint16_t u16_num){
#ifdef MP3_BYPASS
	e_mp3_stat = MP3_DONE;
#else
	if(e_mp3_stat == MP3_IDLE || e_mp3_stat == MP3_DONE){
		if(i_MP3_Start(u16_num) == 0){
			e_mp3_stat = MP3_START;
		}
		else{
			e_mp3_stat = MP3_ERR;
		}
	}
#endif
	return e_mp3_stat;
}


int i_MP3_Play_TT(uint16_t u16_mp3_num);

#define MP3_IF
void v_MP3_Test_Play(){
	static uint32_t timRef, timItv;
	static bool start;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
#ifdef MP3_IF
		if(e_Codec_Ready() == true){
			printf("Audio Succ\n");
#endif

			if(start == false){
				if(i_MP3_Start(0) == 0){
					start = true;
				}
			}
			i_MP3_Decode();
			if(mp3_end){
				mp3_end = false;
				start = false;
				i_MP3_Stop();
				timRef = u32_Tim_1msGet();
				timItv = 5000;
			}

			//i_MP3_Play_TT(0);
#ifdef MP3_IF
		}
#endif
	}
}


void v_MP3_Play_All(){
	static uint32_t timRef, timItv;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
		timRef = u32_Tim_1msGet();

		if(e_Codec_Ready() == true){
			HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);
			v_MP3_Play_T();
			HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);
			timItv = 5000;
		}
		else{
			timItv = 1000;
		}
	}
}


