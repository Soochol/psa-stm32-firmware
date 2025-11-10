#ifndef __JH_LIB_KEY_H
#define __JH_LIB_KEY_H

#include "lib_def.h"

typedef enum {_keyRELEASED=0, _keyPRESSED, _keyLONG_PRESSED} _e_keyREAD_t;
typedef uint32_t (*_u32_Tick)(void);
typedef struct keyTIME{
    uint32_t u32_ref_pressed;
    uint32_t u32_ref_released;
    uint32_t u32_val_pressed;
    uint32_t u32_val_released;
    uint32_t u32_val_long_pressed;
    _u32_Tick	u32_tick;

}_x_keyTIM_t;

_e_keyREAD_t _e_Key_Get(_x_keyTIM_t* px_tim, uint32_t u32_key);
_e_keyREAD_t _e_Key_Get_Long(_x_keyTIM_t* px_tim, uint32_t u32_key);


typedef enum {_keyLOW=0, _keyHIGH, _keyBLINK} _e_keyBLINK_t;
typedef struct {
	struct {
		uint32_t u32_ref;
		uint32_t* pu32_tick;
	}tim;
	_e_keyBLINK_t e_val, e_ref;
	uint8_t u8_low, u8_high;
} _x_keyBLINK_CFG_t;

_e_keyBLINK_t _e_Key_Get_Blink(_x_keyBLINK_CFG_t* px_cfg, uint32_t u32_itv, uint32_t u32_err, uint32_t u32_key);



typedef struct keySET{
	union {
		struct {
			uint8_t b1_rst		:1;
			uint8_t b1_short	:1;
			uint8_t b1_long		:1;
			uint8_t b1_upd		:1;
			uint8_t rsvd		:4;
		}bit;
		uint8_t u8;
	}reg;
	_e_keyREAD_t e_ref;
}_x_keyACT_t;

bool _b_Key_Get_Act(_x_keyACT_t* pe_act, _e_keyREAD_t e_key);


#endif

