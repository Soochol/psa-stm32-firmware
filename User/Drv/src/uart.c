#include "uart.h"
#include "stdio.h"
#include "stdarg.h"
#include "lib_ringbuf.h"
#include "tim.h"

#include "comm_dbg.h"
#include "comm_esp.h"
#include "mode.h"
/****************************************/
//				UART
//	UART1 : to ESP
//	- baud : 115,200
// 	UART3 : rsvd
//	- baud : 115,200
//	UART4 : to DBG
//	- baud : 115,200
//	UART5 : rsvd
//	- baud : 115,200
//	UART8 : rsvd
//	- baud : 115,200
/****************************************/


//extern
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart8;

extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_uart4_tx;



//static
UART_HandleTypeDef* p_uart1 = &huart1;
UART_HandleTypeDef* p_uart3 = &huart3;
UART_HandleTypeDef* p_uart4 = &huart4;
UART_HandleTypeDef* p_uart5 = &huart5;
UART_HandleTypeDef* p_uart8 = &huart8;

DMA_HandleTypeDef* p_dmaUart1Tx = &hdma_usart1_tx;
DMA_HandleTypeDef* p_dmaUart4Tx = &hdma_uart4_tx;


#define UART_CACHE_ENABLED	0

/****************************************/
//	UART1
//	- ESP
/****************************************/
//define
// Must stay a power of two (_RING_VAR_DEF masks with cnt-1).
// 256 B is only ~22 ms of line time at 115200 8N1, so any SD access that blocks
// the main loop longer than that leaves the DMA idle with nothing queued behind
// it. 2 KB covers ~178 ms, which is past the worst case in SD_TIMEOUT_BUSY.
#define UART_ESP_TX_ARR_SIZE	(2048)


//function
static void v_Uart_ESP_Init();
static void v_Uart_ESP_Handler();

//variable
//	TX	//
_RING_VAR_DEF(uartEspTx, uint8_t, UART_ESP_TX_ARR_SIZE);
#if UART_CACHE_ENABLED
ALIGN_32BYTES(static uint8_t u8_txEsp_arr[UART_ESP_TX_ARR_SIZE]); // 32-Byte aligned for cache maintenance
#else
static uint8_t u8_txEsp_arr[UART_ESP_TX_ARR_SIZE + 1] __attribute__((section(".my_nocache_section")));
#endif
//	RX	//
static uint8_t u8_espRx;
// Framing / noise / overrun errors seen on the ESP link, counted in
// HAL_UART_ErrorCallback. Reported alongside the ring counters in comm_esp.c.
static volatile uint32_t u32_espRxErr;

static e_COMM_STAT_t e_espTx;
static e_COMM_STAT_t e_espRx;
/****************************************/
//	UART4
//	- DBG
/****************************************/
//define
#define UART_DBG_ACTIVE			(1)
#define UART_DBG_TX_ARR_SIZE	(512)

//function	: static
static void v_Uart_DBG_Init();
static void v_Uart_DBG_Handler();

//variable	: static
//	TX	//
_RING_VAR_DEF(uartDbgTx, uint8_t, UART_DBG_TX_ARR_SIZE);
#if UART_CACHE_ENABLED
ALIGN_32BYTES(static uint8_t u8_txDbg_arr[UART_DBG_TX_ARR_SIZE]); // 32-Byte aligned for cache maintenance
#else
static uint8_t u8_txDbg_arr[UART_DBG_TX_ARR_SIZE + 1] __attribute__((section(".my_nocache_section")));
#endif
//	RX	//
static uint8_t u8_dbgRxDR[8];

static e_COMM_STAT_t e_dbgTx;
static e_COMM_STAT_t e_dbgRx;

/****************************************/
//	UART - COMMON
//	- interrupt
/****************************************/


/*
 * brief	: uart interrupt complete handler
 * date
 * - create	: 25.03.27
 * - modify	: 25.04.28
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == p_uart1){
		e_espTx = COMM_STAT_DONE;
	}
	else if(huart == p_uart4){
		e_dbgTx = COMM_STAT_DONE;
	}
}


/*
 * brief	: debug receive interrupt
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == p_uart1){
		v_ESP_Recive(u8_espRx);
		// HIGH: Check return value to detect if RX restart fails
		if(HAL_UART_Receive_IT(p_uart1, &u8_espRx, 1) != HAL_OK){
			// If restart fails, communication stops silently - could retry or set error flag
			e_espRx = COMM_STAT_ERR;
		}
	}
	else if(huart == p_uart4){
		//v_DBG_Receive(huart->Instance->RDR);
		v_DBG_Receive(u8_dbgRxDR[0]);
		// HIGH: Check return value to detect if RX restart fails
		if(HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1) != HAL_OK){
			// If restart fails, debug communication stops silently
			e_dbgRx = COMM_STAT_ERR;
		}
	}
}


/*
 * brief	: uart initialize
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void v_Uart_Init(){
	v_Uart_ESP_Init();
	v_Uart_DBG_Init();
}


/*
 * brief	: uart handler
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void v_Uart_Handler(){
	v_Uart_ESP_Handler();
	v_Uart_DBG_Handler();
}


/****************************************/
//	UART1
//	- ESP
/****************************************/

/*
 * brief	: uart initialize for esp
 * datec
 * - create	: 25.04.28
 * - modify	: -
 */
static void v_Uart_ESP_Init(){
	e_espTx = COMM_STAT_READY;
	//	receive		//
	// HIGH: Check return value to detect initialization failure
	if(HAL_UART_Receive_IT(p_uart1, &u8_espRx, 1) != HAL_OK){
		e_espRx = COMM_STAT_ERR;
	}
}

/*
 * brief	: uart error callback - re-arm reception
 * date
 * - create	: 26.09.01
 * note
 * - MANDATORY companion to removing the AbortReceive/Receive_IT pair that used
 *   to bracket every ring-index jump in comm_esp.c. The H7 HAL classifies an
 *   overrun as a blocking error: HAL_UART_IRQHandler calls UART_EndRxTransfer()
 *   and then this callback, leaving RXNEIE clear and RxState READY. Nothing in
 *   the driver re-arms it. Until now that was survivable only by accident --
 *   the parser kept chewing the backlog already in the ring and one of those
 *   jumps called v_Uart_ESP_EnableIT(), which happened to re-arm the receiver.
 *   With the abort gone, an overrun would have killed the ESP link until reset.
 * - Errors are counted, not logged: this runs in the USART ISR.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	if(huart == p_uart1){
		u32_espRxErr++;
		if(HAL_UART_Receive_IT(p_uart1, &u8_espRx, 1) != HAL_OK){
			e_espRx = COMM_STAT_ERR;
		}
	}
	else if(huart == p_uart4){
		if(HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1) != HAL_OK){
			e_dbgRx = COMM_STAT_ERR;
		}
	}
}

uint32_t u32_Uart_ESP_RxErr(void){
	return u32_espRxErr;
}

/*
 * brief	: output
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
bool b_Uart_ESP_Out(uint8_t* pu8_arr, uint16_t u16_cnt){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_cnt == 0){
		return false;
	}

	// The ring drops its oldest bytes on overflow rather than refusing, which
	// would corrupt a frame already in flight. Refuse instead, and let the caller
	// record that this frame never made it out.
	if(((uint32_t)uartEspTx->u16_cnt + u16_cnt) > ((uint32_t)uartEspTx->u16_mask + 1U)){
		return false;
	}

	// HIGH: Protect ring buffer access from ISR race condition
	// Ring buffer shared between main loop and TX complete callback
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	uartEspTx->fn.v_PutArr(uartEspTx, pu8_arr, u16_cnt);
	__set_PRIMASK(primask);

	v_Uart_ESP_TxPump();
	return true;
}

/*
 * brief	: esp handler
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
/*
 * brief	: start the next TX DMA if one is queued and the line is free
 * date
 * - create	: 26.08.11
 * note
 * - Transmit-only on purpose. HAL_UART_TxCpltCallback only flags completion, so
 *   without this being called the queued bytes sit until the main loop comes
 *   back around — which is exactly what does not happen while an SD access
 *   blocks. v_SD_BusyWait_Yield() calls this from inside those waits.
 * - Must NOT touch the receive path. It runs nested inside f_read/f_write, and
 *   dispatching a received command there could re-enter FatFs, which is built
 *   with _FS_REENTRANT = 0.
 */
uint16_t u16_Uart_ESP_TxFree(){
	uint32_t u32_cap = (uint32_t)uartEspTx->u16_mask + 1U;
	uint32_t u32_use = (uint32_t)uartEspTx->u16_cnt;
	return (u32_use >= u32_cap) ? 0U : (uint16_t)(u32_cap - u32_use);
}

void v_Uart_ESP_TxPump(){
	if(uartEspTx->u16_cnt && (e_espTx == COMM_STAT_DONE || e_espTx == COMM_STAT_READY)){
		e_espTx = COMM_STAT_BUSY;
		uint16_t len = uartEspTx->u16_cnt;
		uartEspTx->fn.b_GetArr(uartEspTx, u8_txEsp_arr, len);
#if UART_CACHE_ENABLED
		SCB_CleanDCache_by_Addr((uint32_t*)u8_txEsp_arr, UART_ESP_TX_ARR_SIZE);//after multiple calculation
#endif
		// HIGH: Check return value to detect DMA transmit start failure
		if(HAL_UART_Transmit_DMA(p_uart1, u8_txEsp_arr, len) != HAL_OK){
			e_espTx = COMM_STAT_ERR;
		}
	}
}

static void v_Uart_ESP_Handler(){
	v_Uart_ESP_TxPump();
	v_ESP_Handler();
}




/****************************************/
//	UART4
//	- Debug
/****************************************/

/*
 * brief	: output from printf
 * date
 * - create	: 25.03.26
 * - modify	: 25.04.28
 */
int _write(int file, char* p, int len){
	(void)file;
#if UART_DBG_ACTIVE

	if(e_Mode_Get_CurrID() != modeTEST){
		if(e_dbgTx == COMM_STAT_DONE || e_dbgTx == COMM_STAT_READY){
			v_Uart_DBG_Out((uint8_t*)p, len);
		}
		else{
			uartDbgTx->fn.v_PutArr(uartDbgTx, (uint8_t*)p, len);
		}
	}
#else
	len = 0;
#endif
	return len;
}

/*
 * brief	: uart initialize for debug
 * date
 * - create	: 25.04.16
 * - modify	: 25.04.28
 */
static void v_Uart_DBG_Init(){
	//	transmit	//
	setvbuf(stdout, NULL, _IONBF, 0);	//not buffer
	//setvbuf(stdout, NULL, _IOLBF, 0);	//used buffer : line + \n
	e_dbgTx = COMM_STAT_READY;
	//	receive		//

	// HIGH: Check return value to detect initialization failure
	if(HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1) != HAL_OK){
		e_dbgRx = COMM_STAT_ERR;
	}
	//__HAL_UART_ENABLE_IT(p_uart4, UART_IT_RXNE);
}

void v_Uart_DBG_DisableIT(){
	HAL_UART_AbortReceive(p_uart4);
}

void v_Uart_DBG_EnableIT(){
	// HIGH: Check return value to detect re-enable failure
	if(HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1) != HAL_OK){
		e_dbgRx = COMM_STAT_ERR;
	}
}



/*
 * brief	: uart output from ring buffer
 * date
 * - create	: 25.03.27
 */
void v_Uart_DBG_Out(uint8_t* pu8_arr, uint16_t u16_cnt){
	// CRITICAL: Validate pointer parameters to prevent hard fault
	if(pu8_arr == NULL || u16_cnt == 0){
		return;
	}

	// MEDIUM: Protect ring buffer access from ISR race condition
	// Ring buffer shared between main loop and TX complete callback
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	uartDbgTx->fn.v_PutArr(uartDbgTx, (uint8_t*)pu8_arr, u16_cnt);
	__set_PRIMASK(primask);

	if(e_dbgTx == COMM_STAT_DONE || e_dbgTx == COMM_STAT_READY){
		e_dbgTx = COMM_STAT_BUSY;
		uint16_t len = uartDbgTx->u16_cnt;
		uartDbgTx->fn.b_GetArr(uartDbgTx, u8_txDbg_arr, len);
#if UART_CACHE_ENABLED
		SCB_CleanDCache_by_Addr((uint32_t*)u8_txDbg_arr, UART_DBG_TX_ARR_SIZE);//after multiple calculation'
#endif
		// HIGH: Check return value to detect DMA transmit start failure
		if(HAL_UART_Transmit_DMA(p_uart4, u8_txDbg_arr, len) != HAL_OK){
			e_dbgTx = COMM_STAT_ERR;
		}
	}
}


/*
 * brief	: uart handler
 * date
 * - create	: 25.03.27
 * - modify	: 25.04.28
 */
static void v_Uart_DBG_Handler(){
	if(uartDbgTx->u16_cnt && (e_dbgTx == COMM_STAT_DONE || e_dbgTx == COMM_STAT_READY)){
		e_dbgTx = COMM_STAT_BUSY;
		uint16_t len = uartDbgTx->u16_cnt;
		uartDbgTx->fn.b_GetArr(uartDbgTx, u8_txDbg_arr, len);
#if UART_CACHE_ENABLED
		SCB_CleanDCache_by_Addr((uint32_t*)u8_txDbg_arr, UART_DBG_TX_ARR_SIZE);//after multiple calculation'
#endif
		// HIGH: Check return value to detect DMA transmit start failure
		if(HAL_UART_Transmit_DMA(p_uart4, u8_txDbg_arr, len) != HAL_OK){
			e_dbgTx = COMM_STAT_ERR;
		}
	}
	v_DBG_Handler();
}

/*
 * brief	: transmit complete flag read
 * date
 * - create	: 25.06.20
 */
bool b_Uart_DBG_Ready(){
	if(e_dbgTx == COMM_STAT_DONE || e_dbgTx == COMM_STAT_READY){
		return true;
	}
	else{
		return false;
	}
}


/*
 * brief	: printf output until complete
 * date
 * - create	: 25.06.20
 */
void v_printf_poll(const char *fmt, ...){
#if UART_DBG_ACTIVE
	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);

	va_end(args);

	while(e_dbgTx == COMM_STAT_DONE || e_dbgTx == COMM_STAT_READY);
#endif
}



