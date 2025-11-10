#ifndef __JH_COMM_DBG_H
#define __JH_COMM_DBG_H

#include "uart.h"



void v_DBG_Receive(uint8_t u8_data);
void v_DBG_Handler();

void v_DBG_Transmit(uint8_t u8_cmd, int32_t* pi32_data, uint16_t u16_len);
void v_DBG_BootComplete();

void v_DBG_RxTest();

#endif


