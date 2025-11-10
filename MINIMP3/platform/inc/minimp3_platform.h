#ifndef __JH_MINIMP3_PLATFORM_H
#define __JH_MINIMP3_PLATFORM_H

#include "lib_def.h"
#include "minimp3.h"

#define MP3_CACHE_USED	0


void v_MP3_Init();



void v_MP3_Play_T();

void v_MP3_Test();
int i_MP3_Play(uint16_t u16_mp3_num);


void v_MP3_DAC_Test();


typedef enum {
	MP3_IDLE=0,
	MP3_START,
	MP3_BUSY,
	MP3_DONE,
	MP3_ERR,
} e_MP3_STAT_t;

int i_MP3_Start(uint16_t u16_mp3_num);
int i_MP3_Stop();
int i_MP3_Decode();

int i_MP3_Player(uint16_t u16_num);
int i_MP3_ForceStop();
int i_MP3_Get_Stat();
int i_MP3_Is_Ready();
int i_MP3_Playing();
int i_MP3_Begin(uint16_t u16_num);


void Volume_table_Create();

#endif
