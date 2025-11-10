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
#define UART_ESP_TX_ARR_SIZE	(256)


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

static e_COMM_STAT_t e_espTx;
/****************************************/
//	UART4
//	- DBG
/****************************************/
//define
#define UART_DBG_ACTIVE			(1)
#define UART_DBG_PRINT_TEST		(1)
#define UART_DBG_TX_ARR_SIZE	(512)

//function	: static
static void v_Uart_DBG_Init();
static void v_Uart_DBG_Handler();

static void v_Uart_DBG_Test();

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
		HAL_UART_Receive_IT(p_uart1, &u8_espRx, 1);
	}
	else if(huart == p_uart4){
		//v_DBG_Receive(huart->Instance->RDR);
		v_DBG_Receive(u8_dbgRxDR[0]);
		HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1);
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
	HAL_UART_Receive_IT(p_uart1, &u8_espRx, 1);
}

void v_Uart_ESP_DisableIT(){
	HAL_UART_AbortReceive(p_uart1);
}

void v_Uart_ESP_EnableIT(){
	HAL_UART_Receive_IT(p_uart1, &u8_espRx, 1);
}

/*
 * brief	: output
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
void v_Uart_ESP_Out(uint8_t* pu8_arr, uint16_t u16_cnt){
	uartEspTx->fn.v_PutArr(uartEspTx, pu8_arr, u16_cnt);

	if((e_espTx == COMM_STAT_DONE || e_espTx == COMM_STAT_READY) && uartEspTx->u16_cnt){
		e_espTx = COMM_STAT_BUSY;
		uartEspTx->fn.b_GetArr(uartEspTx, u8_txEsp_arr, u16_cnt);
#if UART_CACHE_ENABLED
		SCB_CleanDCache_by_Addr((uint32_t*)u8_txEsp_arr, UART_ESP_TX_ARR_SIZE);//after multiple calculation
#endif
		HAL_UART_Transmit_DMA(p_uart1, u8_txEsp_arr, u16_cnt);
	}
}

/*
 * brief	: esp handler
 * date
 * - create	: 25.04.28
 * - modify	: -
 */
static void v_Uart_ESP_Handler(){
	if(uartEspTx->u16_cnt && (e_espTx == COMM_STAT_DONE || e_espTx == COMM_STAT_READY)){
		e_espTx = COMM_STAT_BUSY;
		uint16_t len = uartEspTx->u16_cnt;
		uartEspTx->fn.b_GetArr(uartEspTx, u8_txEsp_arr, len);
#if UART_CACHE_ENABLED
		SCB_CleanDCache_by_Addr((uint32_t*)u8_txEsp_arr, UART_ESP_TX_ARR_SIZE);//after multiple calculation
#endif
		HAL_UART_Transmit_DMA(p_uart1, u8_txEsp_arr, len);
	}
	v_ESP_Handler();
	//v_ESP_CmdTest();
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

	HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1);
	//__HAL_UART_ENABLE_IT(p_uart4, UART_IT_RXNE);
}

void v_Uart_DBG_DisableIT(){
	HAL_UART_AbortReceive(p_uart4);
}

void v_Uart_DBG_EnableIT(){
	HAL_UART_Receive_IT(p_uart4, u8_dbgRxDR, 1);
}



/*
 * brief	: uart output from ring buffer
 * date
 * - create	: 25.03.27
 */
void v_Uart_DBG_Out(uint8_t* pu8_arr, uint16_t u16_cnt){
	uartDbgTx->fn.v_PutArr(uartDbgTx, (uint8_t*)pu8_arr, u16_cnt);

	if(e_dbgTx == COMM_STAT_DONE || e_dbgTx == COMM_STAT_READY){
		e_dbgTx = COMM_STAT_BUSY;
		uint16_t len = uartDbgTx->u16_cnt;
		uartDbgTx->fn.b_GetArr(uartDbgTx, u8_txDbg_arr, len);
#if UART_CACHE_ENABLED
		SCB_CleanDCache_by_Addr((uint32_t*)u8_txDbg_arr, UART_DBG_TX_ARR_SIZE);//after multiple calculation'
#endif
		HAL_UART_Transmit_DMA(p_uart4, u8_txDbg_arr, len);
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
		HAL_UART_Transmit_DMA(p_uart4, u8_txDbg_arr, len);
	}
#if UART_DBG_PRINT_TEST
	//v_Uart_DBG_Test();
	//v_DBG_RxTest();
#endif
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


/*
 * brief	: test to terminal from ring buffer
 * date
 * - create	: 25.03.27
 * - modify	: 25.04.28
 */
static void v_Uart_DBG_Test(){
	static uint32_t u32_timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_timRef, 1000)){
		u32_timRef = u32_Tim_1msGet();

		static uint16_t cnt;
		printf("num : %d\n", cnt++);
	}
}




void v_Uart_ESP_Test(){
	static uint32_t timRef;
	static char text = 'a';
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 1000)){
		timRef = u32_Tim_1msGet();
		u8_txEsp_arr[0] = text;
		if(text < 'z')	{text++;}
		else			{text = 'a';}
		HAL_UART_Transmit_DMA(p_uart1, u8_txEsp_arr, 1);
	}
}





