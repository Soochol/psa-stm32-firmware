#include "tim.h"
#include "lib_tim.h"
#include "uart.h"
#include "sk6812_platform.h"
#include "SEGGER_RTT.h"
#include "lib_log.h"
#include "mode.h"
#include "as6221_platform.h"
#include "mlx90640_platform.h"
#include "adc.h"
#include "vl53l0x_platform.h"
//extern
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

extern TIM_HandleTypeDef htim7;

//function
static void v_Tim_SD_Block_Track(void);   // defined below, called from the SD yield hook

//static
static TIM_HandleTypeDef* p_tim2 = &htim2;
static TIM_HandleTypeDef* p_tim4 = &htim4;


static TIM_HandleTypeDef* p_tim7 = &htim7;

/****************************************/
//				SYSTICK					//
//	1ms timer							//
/****************************************/

/*
 * brief	: get 1ms tick counts
 * date
 * - create	: 25.04.16
 * - modify	: -
 */
uint32_t u32_Tim_1msGet(){
	return HAL_GetTick();
}

extern uint32_t one_cycle;

// Mode name lookup for RTT heartbeat
static const char* const MODE_NAMES[] = {
	"BOOT","HEAL","WAIT","F_UP","F_ON","F_DN","SLP","OFF","ERR"
};

void v_Tim_1s_Test(){
	static uint32_t timRef;
	static uint32_t hb_cnt = 0;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 1000)){
		timRef = u32_Tim_1msGet();
		hb_cnt++;
		if(hb_cnt % 5 == 0){
			int mid = (int)e_Mode_Get_CurrID();
			const char* mname = (mid >= 0 && mid < 9) ? MODE_NAMES[mid] : "?";
			SEGGER_RTT_printf(0, "[H]%us %s(%d)\r\n",
				u32_Tim_1msGet()/1000, mname, mid);
			int ti = (int)(f_Temp_In_Get() * 10);
			int to = (int)(f_Temp_Out_Get() * 10);
			int tr = (int)(f_IR_Temp_Get() * 10);
			int bv = (int)(f_ADC_Get_BatVolt() * 10);
			SEGGER_RTT_printf(0, "[T]in=%d.%d out=%d.%d ir=%d.%d\r\n",
				ti/10, ti%10 < 0 ? -(ti%10) : ti%10,
				to/10, to%10 < 0 ? -(to%10) : to%10,
				tr/10, tr%10 < 0 ? -(tr%10) : tr%10);
			unsigned tf = (unsigned)u8_TOF_Get_Frac_1() * 10 / 256;
			SEGGER_RTT_printf(0, "[T]bat=%d.%dV t=%u.%u\r\n",
				bv/10, bv%10 < 0 ? -(bv%10) : bv%10,
				(unsigned)u16_TOF_Get_1(), tf);
			uint32_t us = one_cycle / (SystemCoreClock / 1000000);
			SEGGER_RTT_printf(0, "[D]cyc=%uus sdmax=%uus n=%u\r\n",
				us, (unsigned)u32_Tim_SD_Block_MaxUs(),
				(unsigned)u32_Tim_SD_Block_Count());
			// buckets: <1ms <2 <5 <10 <50 <100 <500 >=500ms
			SEGGER_RTT_printf(0, "[D]sdh %u %u %u %u %u %u %u %u\r\n",
				(unsigned)u32_Tim_SD_Block_Hist(0), (unsigned)u32_Tim_SD_Block_Hist(1),
				(unsigned)u32_Tim_SD_Block_Hist(2), (unsigned)u32_Tim_SD_Block_Hist(3),
				(unsigned)u32_Tim_SD_Block_Hist(4), (unsigned)u32_Tim_SD_Block_Hist(5),
				(unsigned)u32_Tim_SD_Block_Hist(6), (unsigned)u32_Tim_SD_Block_Hist(7));
		}
	}
	v_1Cycle_Time();
}

// Override the weak SD busy-wait yield. Called from every blocking wait in
// sd_diskio.c, so this is the only code that runs while an SD access holds the
// main loop.
//
// Deliberately absent: HAL_IWDG_Refresh(). Feeding the watchdog here would let a
// wedged card stall posture control for the full SD timeout instead of resetting;
// the blocking window is bounded by SD_TIMEOUT_* instead, which is what keeps it
// under the 2 s IWDG period.
//
// Deliberately absent: v_Uart_Handler(). That would dispatch received commands
// and could re-enter FatFs (_FS_REENTRANT = 0). Only the TX side is pumped.
void v_SD_BusyWait_Yield(void) {
	v_Uart_ESP_TxPump();
	v_Tim_SD_Block_Track();
	v_Tim_1s_Test();
}




bool isDWTEnable(){
	return (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);
}

void DWT_Init(){
	if(isDWTEnable() == false){
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;	//DWT used enable
		DWT->CYCCNT = 0;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
	v_Tim_SD_Block_Init();
}


/****************************************/
//		SD BLOCKING MEASUREMENT			//
/****************************************/
// Feeds the scenario 8 report: how long does one SD access hold the main loop?
// v_SD_BusyWait_Yield() is called from every blocking wait in sd_diskio.c and
// those calls sit microseconds apart inside a single wait, so a longer gap means
// a new wait began.
//
// Thresholds are kept in cycles so the hot path does no division. CYCCNT wraps
// every ~7.8 s at 550 MHz; unsigned subtraction absorbs one wrap, and the sanity
// bound catches the case where the card was idle across a wrap and the gap
// happens to read small.
#define SD_BLOCK_GAP_US			2000U		// beyond this, a new wait started
#define SD_BLOCK_SANE_US		2000000U	// longer than IWDG, cannot be one wait

// A single maximum cannot tell a normal tail from a one-off: 600 ms once in ten
// thousand waits and 600 ms every tenth wait look identical. Log-spaced buckets
// make one hardware session answer both, which matters because that session is
// also the SD_TIMEOUT_BUSY decision (spec section 13, scenario 8).
static const uint32_t u32_sdBucketUs[SD_BLOCK_BUCKETS - 1] =
		{1000U, 2000U, 5000U, 10000U, 50000U, 100000U, 500000U};

static uint32_t u32_sdBucketCyc[SD_BLOCK_BUCKETS - 1];
static uint32_t u32_sdHist[SD_BLOCK_BUCKETS];
static uint32_t u32_sdWaitCnt;
static uint32_t u32_sdBlockMaxCyc;
static uint32_t u32_sdBlockStart;
static uint32_t u32_sdBlockLast;
static uint32_t u32_sdGapCyc;
static uint32_t u32_sdSaneCyc;

void v_Tim_SD_Block_Init(void){
	uint32_t u32_perUs = SystemCoreClock / 1000000U;
	u32_sdGapCyc  = SD_BLOCK_GAP_US  * u32_perUs;
	u32_sdSaneCyc = SD_BLOCK_SANE_US * u32_perUs;
	for(int i = 0; i < SD_BLOCK_BUCKETS - 1; i++){
		u32_sdBucketCyc[i] = u32_sdBucketUs[i] * u32_perUs;
	}
}

/* Called when a wait is seen to have ended, with its duration in cycles. */
static void v_sd_bucket(uint32_t u32_cyc){
	int i = 0;
	while(i < SD_BLOCK_BUCKETS - 1 && u32_cyc >= u32_sdBucketCyc[i]) i++;
	u32_sdHist[i]++;
	u32_sdWaitCnt++;
}

static void v_Tim_SD_Block_Track(void){
	uint32_t u32_now  = DWT->CYCCNT;
	uint32_t u32_gap  = u32_now - u32_sdBlockLast;
	uint32_t u32_held = u32_now - u32_sdBlockStart;

	if(u32_gap > u32_sdGapCyc || u32_held > u32_sdSaneCyc){
		// The gap means the previous wait finished at u32_sdBlockLast. Bucket it
		// now; the very last wait before a report is counted on the next SD access.
		if(u32_sdWaitCnt || u32_sdBlockLast != u32_sdBlockStart){
			v_sd_bucket(u32_sdBlockLast - u32_sdBlockStart);
		}
		u32_sdBlockStart = u32_now;
	}
	else if(u32_held > u32_sdBlockMaxCyc){
		u32_sdBlockMaxCyc = u32_held;
	}
	u32_sdBlockLast = u32_now;
}

uint32_t u32_Tim_SD_Block_MaxUs(void){
	uint32_t u32_perUs = SystemCoreClock / 1000000U;
	if(u32_perUs == 0U) return 0U;
	return u32_sdBlockMaxCyc / u32_perUs;
}

uint32_t u32_Tim_SD_Block_Hist(uint8_t u8_bucket){
	return (u8_bucket < SD_BLOCK_BUCKETS) ? u32_sdHist[u8_bucket] : 0U;
}

uint32_t u32_Tim_SD_Block_Count(void){
	return u32_sdWaitCnt;
}

void v_Tim_SD_Block_Clear(void){
	u32_sdBlockMaxCyc = 0;
	u32_sdWaitCnt = 0;
	for(int i = 0; i < SD_BLOCK_BUCKETS; i++) u32_sdHist[i] = 0;
}

void delay_us(uint32_t us){
	uint32_t start = DWT->CYCCNT;
	uint32_t ticks = us * (SystemCoreClock / 1000000);

	while((DWT->CYCCNT - start) < ticks);
}

uint32_t one_cycle;
uint32_t one_cycle_time;
void v_1Cycle_Time(){
	static uint32_t start;

	uint32_t end = DWT->CYCCNT;
	if(end >= start){
		one_cycle = end - start;
	}
	else{
		one_cycle = 0xFFFFFFFF - start + end;
	}
	//one_cycle_time = one_cycle *

	start = DWT->CYCCNT;
}


/****************************************/
//				TIM1					//
//	Update event - TRGO					//
//	clock : 192,000,000 Hz				//
//	prescaler : 1						//
//	counts	: 8000						//
//	overflow : 24,000 Hz				//
/****************************************/
void v_TIM1_Init(){

}




/****************************************/
//				TIM2					//
//	CH1	: PWM (actuator heater)			//
//	CH2	: PWM (fan)						//
//	CH3	: PWM (heat pad)				//
//	CH4 : PWM (actuator cool fan)		//
//	clock : 192,000,000 Hz				//
//	prescaler : 1920					//
//	counts	: 1000						//
//	overflow : 100 Hz					//
/****************************************/
#define TIM2_ARR_MAX	1000


/*
 * brief	: TIM2 CH1 pwm output
 * date
 * - create	: 25.07.25
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch1_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev = 0xFFFF;

	// MEDIUM: Validate PWM value to prevent undefined behavior
	if(u16_pwm > TIM2_ARR_MAX){
		u16_pwm = TIM2_ARR_MAX;
	}

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_1, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_1);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_1);
			on = false;
		}
	}
}


/*
 * brief	: TIM2 CH2 pwm output
 * date
 * - create	: 25.07.16
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch2_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev = 0xFFFF;

	// MEDIUM: Validate PWM value to prevent undefined behavior
	if(u16_pwm > TIM2_ARR_MAX){
		u16_pwm = TIM2_ARR_MAX;
	}

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_2, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_2);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_2);
			on = false;
		}
	}
}

/*
 * brief	: TIM2 CH3 pwm output
 * date
 * - create	: 25.04.16
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch3_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev = 0xFFFF;

	// MEDIUM: Validate PWM value to prevent undefined behavior
	if(u16_pwm > TIM2_ARR_MAX){
		u16_pwm = TIM2_ARR_MAX;
	}

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_3, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_3);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_3);
			on = false;
		}
	}
}



/*
 * brief	: TIM2 CH4 pwm output
 * date
 * - create	: 25.04.16
 * - modify	:
 * note
 * - PWM output
 */
void v_TIM2_Ch4_Out(uint16_t u16_pwm){
	static bool on;
	static uint16_t pwm_prev;

	// MEDIUM: Validate PWM value to prevent undefined behavior
	if(u16_pwm > TIM2_ARR_MAX){
		u16_pwm = TIM2_ARR_MAX;
	}

	if(pwm_prev != u16_pwm){
		pwm_prev = u16_pwm;

		__HAL_TIM_SET_COMPARE(p_tim2, TIM_CHANNEL_4, u16_pwm);

		if(u16_pwm){
			if(on == false){
				on = true;
				HAL_TIM_PWM_Start(p_tim2, TIM_CHANNEL_4);
			}
		}
		else{
			HAL_TIM_PWM_Stop(p_tim2, TIM_CHANNEL_4);
			on = false;
		}
	}
}


/****************************************/
//				TIM4					//
//	CH2	: PWM (Digital RGB)				//
//	clock : 192,000,000 Hz				//
//	prescaler : 1						//
//	counts	: 240						//
//	overflow : 800,000 Hz				//
/****************************************/
#include "lib_ringbuf.h"

#define TIM4_LED_SIZE	512
_RING_VAR_DEF(drgb, uint16_t, TIM4_LED_SIZE);

static bool b_tim4_ready;
uint16_t led_send_cnt;

int led_out;

#define TIM4_CACHE_ENABLE	0
#if TIM4_CACHE_ENABLE
ALIGN_32BYTES(static volatile uint16_t u16_tim4_pwm[TIM4_LED_SIZE] __attribute__((section(".my_d2_section")))); // 32-Byte aligned for cache maintenance
#else
static volatile uint16_t u16_tim4_pwm[TIM4_LED_SIZE] __attribute__((section(".my_nocache_section"))); // 32-Byte aligned for cache maintenance
#endif

void v_Tim4_Ch2_Out(uint16_t* pu16_pwmArr, uint16_t u16_cnt){
	if(b_tim4_ready == false){

		//HAL_TIM_PWM_Stop_DMA(p_tim4, TIM_CHANNEL_2);

		__HAL_TIM_CLEAR_FLAG(p_tim4, TIM_FLAG_CC2);
		__HAL_TIM_CLEAR_FLAG(p_tim4, TIM_FLAG_UPDATE);

		__HAL_TIM_DISABLE_OCxPRELOAD(p_tim4, TIM_CHANNEL_2);
		__HAL_TIM_SET_COMPARE(p_tim4, TIM_CHANNEL_2, 0);
		//__HAL_TIM_SET_COUNTER(p_tim4, 0);


		led_send_cnt = u16_cnt;
		for(int i=0; i<u16_cnt; i++){
			u16_tim4_pwm[i] = pu16_pwmArr[i];
		}
#if TIM4_CACHE_ENABLE
		SCB_CleanDCache_by_Addr((uint32_t*)&u16_tim4_pwm[0], sizeof(uint16_t) * TIM4_LED_SIZE);//after multiple calculation
#endif
		HAL_TIM_PWM_Start_DMA(p_tim4, TIM_CHANNEL_2, (uint32_t*)u16_tim4_pwm, u16_cnt + 1);	//cnt + 1 -> add low output
		__HAL_TIM_ENABLE_OCxPRELOAD(p_tim4, TIM_CHANNEL_2);
		b_tim4_ready = true;
	}
}






/****************************************/
//				TIM7					//
//	CH2	: PWM (Digital RGB)				//
//	clock : 192,000,000 Hz				//
//	prescaler : 1						//
//	counts	: 192						//
//	reload	: 100						//
//	overflow : 10,000 Hz				//
/****************************************/
uint32_t PCLK2;
void v_Tim_Init(){
	b_tim4_ready = false;
	HAL_TIM_Base_Start_IT(p_tim7);
	for(int i=0; i<sizeof(u16_tim4_pwm); i++){
		u16_tim4_pwm[0] = 0;
	}
	PCLK2 = HAL_RCC_GetPCLK2Freq();
}

void v_Tim_Handler(){

}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM7){
		v_RGB_Done_Handler();
	}
}



void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM4){
		b_tim4_ready = false;
		HAL_TIM_PWM_Stop_DMA(p_tim4, TIM_CHANNEL_2);
	}
}







