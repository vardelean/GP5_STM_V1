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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOF
#define USB_PWR_Pin GPIO_PIN_0
#define USB_PWR_GPIO_Port GPIOC
#define btnScene1_Pin GPIO_PIN_1
#define btnScene1_GPIO_Port GPIOC
#define btnScene2_Pin GPIO_PIN_2
#define btnScene2_GPIO_Port GPIOC
#define btnScene3_Pin GPIO_PIN_3
#define btnScene3_GPIO_Port GPIOC
#define ledScene1_Pin GPIO_PIN_0
#define ledScene1_GPIO_Port GPIOA
#define ledScene2_Pin GPIO_PIN_1
#define ledScene2_GPIO_Port GPIOA
#define USART2_TX_Pin GPIO_PIN_2
#define USART2_TX_GPIO_Port GPIOA
#define USART2_RX_Pin GPIO_PIN_3
#define USART2_RX_GPIO_Port GPIOA
#define ledScene3_Pin GPIO_PIN_4
#define ledScene3_GPIO_Port GPIOA
#define LED_GREEN_Pin GPIO_PIN_5
#define LED_GREEN_GPIO_Port GPIOA
#define btnUp_Pin GPIO_PIN_4
#define btnUp_GPIO_Port GPIOC
#define btnDown_Pin GPIO_PIN_5
#define btnDown_GPIO_Port GPIOC
#define btnTap_Pin GPIO_PIN_6
#define btnTap_GPIO_Port GPIOC
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
