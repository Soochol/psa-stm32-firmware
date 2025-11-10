#ifndef __JH_AS6221_TRANSPORT_H
#define __JH_AS6221_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif



typedef int (*_fn_i_AS6221_Write)(uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len);
typedef int (*_fn_i_AS6221_Read)(uint8_t u8_reg, uint8_t* pu8_arr, uint16_t u16_len);
typedef int (*_fn_i_AS6221_Bus)(void);
typedef int (*_fn_i_AS6221_Proc)(void);

typedef struct {
	_fn_i_AS6221_Write	i_write;
	_fn_i_AS6221_Read	i_read;
	_fn_i_AS6221_Bus	i_bus;
	_fn_i_AS6221_Proc	i_proc;
} _x_AS6221_TRANS_t;




#ifdef __cplusplus
}
#endif

#endif
