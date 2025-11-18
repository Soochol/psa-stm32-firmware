#include "io.h"
#include "vl53l0x_platform.h"
#include "lib_log.h"

static void v_IO_LED_Disable();

uint16_t v12_toggle;
void v_IO_Check_12V(){
	static GPIO_PinState before = GPIO_PIN_SET;
	GPIO_PinState now;
	now = HAL_GPIO_ReadPin(DO_12VA_EN_GPIO_Port, DO_12VA_EN_Pin);

	if(now != before){
		before = now;
		v12_toggle++;
	}
}

/*
 * brief	: 12VA enable
 */
void v_IO_Enable_12V(){
	HAL_GPIO_WritePin(DO_12VA_EN_GPIO_Port, DO_12VA_EN_Pin, GPIO_PIN_SET);
}

void v_IO_Disable_12V(){
	HAL_GPIO_WritePin(DO_12VA_EN_GPIO_Port, DO_12VA_EN_Pin, GPIO_PIN_RESET);
}

/*
 * brief	: heat pad enable
 */
void v_IO_Enable_HeatPad(){
	HAL_GPIO_WritePin(DO_PAD_EN_GPIO_Port, DO_PAD_EN_Pin, GPIO_PIN_SET);
}

// LOW: Fixed typo (HeadPad -> HeatPad)
void v_IO_Disable_HeatPad(){
	HAL_GPIO_WritePin(DO_PAD_EN_GPIO_Port, DO_PAD_EN_Pin, GPIO_PIN_RESET);
}

/*
 * brief	: fan enable
 */
void v_IO_Enable_Fan(){
	HAL_GPIO_WritePin(DO_FAN_EN_GPIO_Port, DO_FAN_EN_Pin, GPIO_PIN_SET);
}

void v_IO_Disable_Fan(){
	HAL_GPIO_WritePin(DO_FAN_EN_GPIO_Port, DO_FAN_EN_Pin, GPIO_PIN_RESET);
}



void v_IO_Init(){
	v_IO_LED_Disable();
}

static void v_IO_LED_Disable(){
	HAL_GPIO_WritePin(DO_EXT_IO1_GPIO_Port, DO_EXT_IO1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DO_EXT_IO2_GPIO_Port, DO_EXT_IO2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DO_EXT_IO3_GPIO_Port, DO_EXT_IO3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DO_EXT_IO4_GPIO_Port, DO_EXT_IO4_Pin, GPIO_PIN_RESET);
}




void v_AUDIO_Init(){
	__HAL_RCC_GPIOE_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// 1. SCL, SDA를 GPIO Output으로 설정
	GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | DO_I2C5_ADD_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);
	HAL_GPIO_DeInit(GPIOE, GPIO_PIN_3);
	HAL_GPIO_DeInit(GPIOE, GPIO_PIN_4);
	HAL_GPIO_DeInit(GPIOE, GPIO_PIN_5);
	HAL_GPIO_DeInit(GPIOE, GPIO_PIN_6);
	HAL_GPIO_DeInit(GPIOE, DO_I2C5_ADD_Pin);
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);


	HAL_GPIO_WritePin(DO_I2C5_ADD_GPIO_Port, DO_I2C5_ADD_Pin, GPIO_PIN_RESET);
	//AUDIO
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
}


void v_I2C1_Pin_Deinit(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//	PB6, PB7
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
}

void v_I2C2_Pin_Deinit(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//	PB10, PB11
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
}

void v_I2C3_Pin_Deinit(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//	PA8
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	//	PC9
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
}

void v_I2C4_Pin_Deinit(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//	PB8, PB9
	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);



	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);


}

void v_I2C5_Pin_Deinit(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//	PC10, PC11
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10);
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_11);
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
}

/*
 * brief	: I2C bus diagnostic dump
 * note		: Prints state and error codes for all I2C buses
 */
void v_I2C_DiagDump(void) {
	extern I2C_HandleTypeDef hi2c1, hi2c2, hi2c3, hi2c4, hi2c5;

	LOG_ERROR("I2C", "Bus Status Dump:");
	LOG_ERROR("I2C", "I2C1: State=0x%02X, Error=0x%08lX, ISR=0x%08lX",
	          hi2c1.State, hi2c1.ErrorCode, hi2c1.Instance->ISR);
	LOG_ERROR("I2C", "I2C2: State=0x%02X, Error=0x%08lX, ISR=0x%08lX",
	          hi2c2.State, hi2c2.ErrorCode, hi2c2.Instance->ISR);
	LOG_ERROR("I2C", "I2C3: State=0x%02X, Error=0x%08lX, ISR=0x%08lX",
	          hi2c3.State, hi2c3.ErrorCode, hi2c3.Instance->ISR);
	LOG_ERROR("I2C", "I2C4: State=0x%02X, Error=0x%08lX, ISR=0x%08lX",
	          hi2c4.State, hi2c4.ErrorCode, hi2c4.Instance->ISR);
	LOG_ERROR("I2C", "I2C5: State=0x%02X, Error=0x%08lX, ISR=0x%08lX",
	          hi2c5.State, hi2c5.ErrorCode, hi2c5.Instance->ISR);
}

/**
 * @brief Check if I2C device is ready (responds with ACK)
 * @param p_i2c_handle Pointer to I2C_HandleTypeDef
 * @param bus_num I2C bus number (for logging)
 * @param dev_addr Device 7-bit address (will be shifted left internally by HAL)
 * @param dev_name Device name string (for logging)
 * @return 0 if ACK, -1 if NACK or error
 */
int i_I2C_ProbeDevice(void* p_i2c_handle, uint8_t bus_num, uint8_t dev_addr, const char* dev_name) {
	I2C_HandleTypeDef *p_i2c = (I2C_HandleTypeDef*)p_i2c_handle;

	// dev_addr is already shifted (8-bit format), display 7-bit address for logging
	LOG_DEBUG("I2C", "I2C%d_PROBE: Checking %s (addr=0x%02X)...", bus_num, dev_name, dev_addr >> 1);

	// dev_addr already in 8-bit format, use as-is
	HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(p_i2c, dev_addr, 3, 100);

	if(ret == HAL_OK) {
		LOG_DEBUG("I2C", "I2C%d_PROBE: %s ACK \u2713", bus_num, dev_name);
		return 0;
	} else {
		LOG_DEBUG("I2C", "I2C%d_PROBE: %s NACK \u2717 (HAL_Status=%d)", bus_num, dev_name, ret);
		return -1;
	}
}



void v_IO_PWR_WakeUp_Enable(){
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = EXTI14_PWR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_DeInit(EXTI14_PWR_GPIO_Port, EXTI14_PWR_Pin);
	HAL_GPIO_Init(EXTI14_PWR_GPIO_Port, &GPIO_InitStruct);
}

void v_IO_PWR_WakeUp_Disable(){
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = EXTI14_PWR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_DeInit(EXTI14_PWR_GPIO_Port, EXTI14_PWR_Pin);
	HAL_GPIO_Init(EXTI14_PWR_GPIO_Port, &GPIO_InitStruct);
}









