#ifndef __JH_LIB_MODE_H
#define __JH_LIB_MODE_H

#include "lib_def.h"

#define _modeCR_UPD_MASK	0x01
#define _modeCR_ON_MASK		0x02
#define _modeCR_OFF_MASK	0x04
#define _modeCR_STOP_MASK	0x08

typedef struct {
	union {
		struct {
			uint8_t b1_upd	:1;
			uint8_t b1_on	:1;
			uint8_t b1_off	:1;
			uint8_t b1_stop	:1;
			uint8_t rsvd	:4;
		}bit;
		uint8_t u8;
	}cr;
	struct {
		void (*v_next)(void*);
		void (*v_curr)(void*);
	}fn;
} _x_modeCFG_t;


void _v_Mode_Set_Next(_x_modeCFG_t* px_cfg);
void _v_Mode_Next(_x_modeCFG_t* px_cfg);
bool _b_Mode_Run(_x_modeCFG_t* px_cfg, void* pv_param);


#endif


