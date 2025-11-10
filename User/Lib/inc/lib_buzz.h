#ifndef __JH_LIB_BUZZ_H
#define __JH_LIB_BUZZ_H

#include "lib_def.h"


typedef enum {_buzzIDLE=0, _buzzPLAY, _buzzDONE, _buzzDELAY, _buzzSTOP} _e_buzzSTAT_t;
typedef struct {
	struct {
		void (*v_Play)(bool b_play, void* pv_param);
		bool (*b_IsDone)(void);
	}fn;
	struct {
		uint32_t* pu32_tick;
		uint32_t u32_ref;
		uint32_t u32_delay;
	}tim;
	_e_buzzSTAT_t e_stat;
	uint8_t u8_play_cnt;
	void* pv_play_param;
} _x_buzzCFG_t;


void _v_Buzz_Init(_x_buzzCFG_t* px_cfg, uint32_t* pu32_tick, void(*v_Play)(bool, void*), bool(*b_IsDone)(void));
bool _b_Buzz_Set_CFG(_x_buzzCFG_t* px_cfg, void* pv_play_param, uint8_t u8_play_cnt, uint32_t u32_delay);
void _v_Buzz_Stop(_x_buzzCFG_t* px_cfg);
void _v_Buzz_Proc(_x_buzzCFG_t* px_cfg);

//bool _b_Buzz_SetCFG_ThenStart();
#endif


