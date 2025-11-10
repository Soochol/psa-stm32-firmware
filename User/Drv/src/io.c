#include "io.h"
#include "vl53l0x_platform.h"

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

void v_IO_Disable_HeadPad(){
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









