#ifndef __JH_UART_H
#define __JH_UART_H

#include "main.h"
#include "lib_def.h"


/****************************************/
//	UART - COMMON						//
/****************************************/
void v_Uart_Init();
void v_Uart_Handler();

/****************************************/
//	UART1 - ESP							//
/****************************************/
void v_Uart_ESP_DisableIT();
void v_Uart_ESP_EnableIT();

void v_Uart_ESP_Out(uint8_t* pu8_arr, uint16_t u16_cnt);


/****************************************/
//	UART4 - Debug						//
/****************************************/
void v_Uart_DBG_DisableIT();
void v_Uart_DBG_EnableIT();

void v_Uart_DBG_Out(uint8_t* pu8_arr, uint16_t u16_cnt);

bool b_Uart_DBG_Ready();


void v_printf_poll(const char *fmt, ...);




void v_Uart_ESP_Test();


#endif


