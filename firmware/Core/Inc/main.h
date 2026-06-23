/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SWITCH_Pin GPIO_PIN_9
#define SWITCH_GPIO_Port GPIOB
#define SWITCH_EXTI_IRQn EXTI4_15_IRQn
#define FLT_HOSTB_VBUSn_Pin GPIO_PIN_14
#define FLT_HOSTB_VBUSn_GPIO_Port GPIOC
#define FLT_HOSTA_VBUSn_Pin GPIO_PIN_15
#define FLT_HOSTA_VBUSn_GPIO_Port GPIOC
#define HOSTA_VBUS_MEAS_Pin GPIO_PIN_0
#define HOSTA_VBUS_MEAS_GPIO_Port GPIOA
#define HOSTB_VBUS_MEAS_Pin GPIO_PIN_1
#define HOSTB_VBUS_MEAS_GPIO_Port GPIOA
#define DS1_CURSENSE_Pin GPIO_PIN_4
#define DS1_CURSENSE_GPIO_Port GPIOA
#define DS2_CURSENSE_Pin GPIO_PIN_5
#define DS2_CURSENSE_GPIO_Port GPIOA
#define DS3_CURSENSE_Pin GPIO_PIN_6
#define DS3_CURSENSE_GPIO_Port GPIOA
#define DS_VBUS_MEAS_Pin GPIO_PIN_7
#define DS_VBUS_MEAS_GPIO_Port GPIOA
#define LEDBn_Pin GPIO_PIN_0
#define LEDBn_GPIO_Port GPIOB
#define LEDGn_Pin GPIO_PIN_1
#define LEDGn_GPIO_Port GPIOB
#define HOSTMUX_OEn_Pin GPIO_PIN_2
#define HOSTMUX_OEn_GPIO_Port GPIOB
#define PGANG_Pin GPIO_PIN_8
#define PGANG_GPIO_Port GPIOA
#define I2C_SCL_UI_Pin GPIO_PIN_9
#define I2C_SCL_UI_GPIO_Port GPIOA
#define LEDRn_Pin GPIO_PIN_6
#define LEDRn_GPIO_Port GPIOC
#define I2C_SDA_UI_Pin GPIO_PIN_10
#define I2C_SDA_UI_GPIO_Port GPIOA
#define MCU_DM_Pin GPIO_PIN_11
#define MCU_DM_GPIO_Port GPIOA
#define MCU_DP_Pin GPIO_PIN_12
#define MCU_DP_GPIO_Port GPIOA
#define RESETn_Pin GPIO_PIN_15
#define RESETn_GPIO_Port GPIOA
#define HOSTMUX_SEL_Pin GPIO_PIN_3
#define HOSTMUX_SEL_GPIO_Port GPIOB
#define ST_HUB_VBUS_Pin GPIO_PIN_4
#define ST_HUB_VBUS_GPIO_Port GPIOB
#define EN_VBUSB_Pin GPIO_PIN_5
#define EN_VBUSB_GPIO_Port GPIOB
#define EN_VBUSA_Pin GPIO_PIN_8
#define EN_VBUSA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define CONSOLE_UART 1
#define CONSOLE_USB  2
#define CONSOLE_TYPE CONSOLE_UART
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
