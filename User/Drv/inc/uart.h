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

// false when the ring has no room. The ring overwrites its oldest bytes rather
// than refusing, which would corrupt a frame mid-flight, so the check happens
// here and the caller decides what a dropped frame means.
bool b_Uart_ESP_Out(uint8_t* pu8_arr, uint16_t u16_cnt);

// Transmit-only drain, safe to call from inside a blocking SD wait.
// Does not touch the receive path — see the note at the definition.
void v_Uart_ESP_TxPump(void);

// Room left in the TX ring. Backfill uses it to leave headroom for the live
// STAT frame, which must never queue behind a burst of log chunks.
uint16_t u16_Uart_ESP_TxFree(void);


/****************************************/
//	UART4 - Debug						//
/****************************************/
void v_Uart_DBG_DisableIT();
void v_Uart_DBG_EnableIT();

void v_Uart_DBG_Out(uint8_t* pu8_arr, uint16_t u16_cnt);

bool b_Uart_DBG_Ready();


void v_printf_poll(const char *fmt, ...);



#endif


