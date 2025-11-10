/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void v_RTC_Write_BKUP(uint32_t u32_wr);
uint32_t u32_RTC_Read_BKUP();
void SystemClock_Config(void);
void v_WakeUp_Clock_Config();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DO_12VA_EN_Pin GPIO_PIN_13
#define DO_12VA_EN_GPIO_Port GPIOC
#define DO_PAD_EN_Pin GPIO_PIN_14
#define DO_PAD_EN_GPIO_Port GPIOC
#define DO_FAN_EN_Pin GPIO_PIN_15
#define DO_FAN_EN_GPIO_Port GPIOC
#define ADC1_INP10_HEATER_Pin GPIO_PIN_0
#define ADC1_INP10_HEATER_GPIO_Port GPIOC
#define ADC1_INP11_VBAT_Pin GPIO_PIN_1
#define ADC1_INP11_VBAT_GPIO_Port GPIOC
#define ADC3_INP0_FAN_Pin GPIO_PIN_2
#define ADC3_INP0_FAN_GPIO_Port GPIOC
#define ADC3_INP1_PAD_Pin GPIO_PIN_3
#define ADC3_INP1_PAD_GPIO_Port GPIOC
#define TIM2_CH1_HEATER_Pin GPIO_PIN_0
#define TIM2_CH1_HEATER_GPIO_Port GPIOA
#define TIM2_CH2_BLOWFAN_Pin GPIO_PIN_1
#define TIM2_CH2_BLOWFAN_GPIO_Port GPIOA
#define TIM2_CH3_PAD_Pin GPIO_PIN_2
#define TIM2_CH3_PAD_GPIO_Port GPIOA
#define TIM2_CH4_ACT_COOLFAN_Pin GPIO_PIN_3
#define TIM2_CH4_ACT_COOLFAN_GPIO_Port GPIOA
#define DAC1_OUT1_AUDIO_Pin GPIO_PIN_4
#define DAC1_OUT1_AUDIO_GPIO_Port GPIOA
#define DAC1_OUT2_AUDIO_Pin GPIO_PIN_5
#define DAC1_OUT2_AUDIO_GPIO_Port GPIOA
#define ADC1_INP3_ACT_COOLFAN_Pin GPIO_PIN_6
#define ADC1_INP3_ACT_COOLFAN_GPIO_Port GPIOA
#define DI_ACT_TOF1_GPIO_Pin GPIO_PIN_7
#define DI_ACT_TOF1_GPIO_GPIO_Port GPIOA
#define DO_ACT_TOF1_SHUT_Pin GPIO_PIN_4
#define DO_ACT_TOF1_SHUT_GPIO_Port GPIOC
#define DI_IMUR_INT_Pin GPIO_PIN_1
#define DI_IMUR_INT_GPIO_Port GPIOB
#define DI_IMUL_INT_Pin GPIO_PIN_2
#define DI_IMUL_INT_GPIO_Port GPIOB
#define DI_TOF2_GPIO_Pin GPIO_PIN_7
#define DI_TOF2_GPIO_GPIO_Port GPIOE
#define DO_TOF2_SHUT_Pin GPIO_PIN_8
#define DO_TOF2_SHUT_GPIO_Port GPIOE
#define DI_CTRL_SW1_Pin GPIO_PIN_9
#define DI_CTRL_SW1_GPIO_Port GPIOE
#define DO_AUDIO_SHDN_Pin GPIO_PIN_10
#define DO_AUDIO_SHDN_GPIO_Port GPIOE
#define DO_I2C5_ADD_Pin GPIO_PIN_11
#define DO_I2C5_ADD_GPIO_Port GPIOE
#define I2C2_SCL_SUB_Pin GPIO_PIN_10
#define I2C2_SCL_SUB_GPIO_Port GPIOB
#define I2C2_SDA_SUB_Pin GPIO_PIN_11
#define I2C2_SDA_SUB_GPIO_Port GPIOB
#define UART5_RX_RSVD_Pin GPIO_PIN_12
#define UART5_RX_RSVD_GPIO_Port GPIOB
#define UART5_TX_RSVD_Pin GPIO_PIN_13
#define UART5_TX_RSVD_GPIO_Port GPIOB
#define UART3_TX_RSVD_Pin GPIO_PIN_8
#define UART3_TX_RSVD_GPIO_Port GPIOD
#define UART3_RX_RSVD_Pin GPIO_PIN_9
#define UART3_RX_RSVD_GPIO_Port GPIOD
#define DO_CTRL_LED2_Pin GPIO_PIN_12
#define DO_CTRL_LED2_GPIO_Port GPIOD
#define TIM4_CH2_CTRL_LED1_Pin GPIO_PIN_13
#define TIM4_CH2_CTRL_LED1_GPIO_Port GPIOD
#define EXTI14_PWR_Pin GPIO_PIN_14
#define EXTI14_PWR_GPIO_Port GPIOD
#define EXTI14_PWR_EXTI_IRQn EXTI15_10_IRQn
#define DI_CTRL_SW2_Pin GPIO_PIN_15
#define DI_CTRL_SW2_GPIO_Port GPIOD
#define UART1_TX_ESP_Pin GPIO_PIN_9
#define UART1_TX_ESP_GPIO_Port GPIOA
#define UART1_RX_ESP_Pin GPIO_PIN_10
#define UART1_RX_ESP_GPIO_Port GPIOA
#define UART4_RX_DBG_Pin GPIO_PIN_11
#define UART4_RX_DBG_GPIO_Port GPIOA
#define UART4_TX_DBG_Pin GPIO_PIN_12
#define UART4_TX_DBG_GPIO_Port GPIOA
#define I2C5_SDA_AUDIO_Pin GPIO_PIN_10
#define I2C5_SDA_AUDIO_GPIO_Port GPIOC
#define I2C5_SCL_AUDIO_Pin GPIO_PIN_11
#define I2C5_SCL_AUDIO_GPIO_Port GPIOC
#define DO_EXT_IO1_Pin GPIO_PIN_0
#define DO_EXT_IO1_GPIO_Port GPIOD
#define DO_EXT_IO2_Pin GPIO_PIN_1
#define DO_EXT_IO2_GPIO_Port GPIOD
#define DO_EXT_IO3_Pin GPIO_PIN_2
#define DO_EXT_IO3_GPIO_Port GPIOD
#define DO_EXT_IO4_Pin GPIO_PIN_3
#define DO_EXT_IO4_GPIO_Port GPIOD
#define DI_DET_SD_Pin GPIO_PIN_5
#define DI_DET_SD_GPIO_Port GPIOD
#define I2C1_SCL_ACT_Pin GPIO_PIN_6
#define I2C1_SCL_ACT_GPIO_Port GPIOB
#define I2C1_SDA_ACT_Pin GPIO_PIN_7
#define I2C1_SDA_ACT_GPIO_Port GPIOB
#define I2C4_SCL_ACT_Pin GPIO_PIN_8
#define I2C4_SCL_ACT_GPIO_Port GPIOB
#define I2C4_SDA_ACT_Pin GPIO_PIN_9
#define I2C4_SDA_ACT_GPIO_Port GPIOB
#define UART8_RX_RSVD_Pin GPIO_PIN_0
#define UART8_RX_RSVD_GPIO_Port GPIOE
#define UART8_TX_RSVD_Pin GPIO_PIN_1
#define UART8_TX_RSVD_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
