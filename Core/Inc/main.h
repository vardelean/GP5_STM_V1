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
#define USART2_TX_Pin GPIO_PIN_2
#define USART2_TX_GPIO_Port GPIOA
#define USART2_RX_Pin GPIO_PIN_3
#define USART2_RX_GPIO_Port GPIOA
#define btnPst0_Pin GPIO_PIN_0
#define btnPst0_GPIO_Port GPIOB
#define btnPst1_Pin GPIO_PIN_1
#define btnPst1_GPIO_Port GPIOB
#define btnPst2_Pin GPIO_PIN_2
#define btnPst2_GPIO_Port GPIOB
#define USB_PWR_Pin GPIO_PIN_6
#define USB_PWR_GPIO_Port GPIOC
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define btnPst3_Pin GPIO_PIN_3
#define btnPst3_GPIO_Port GPIOB
#define btnPst4_Pin GPIO_PIN_4
#define btnPst4_GPIO_Port GPIOB
#define btnBankUp_Pin GPIO_PIN_5
#define btnBankUp_GPIO_Port GPIOB
#define btnBankDown_Pin GPIO_PIN_6
#define btnBankDown_GPIO_Port GPIOB
#define btnCtl_Pin GPIO_PIN_7
#define btnCtl_GPIO_Port GPIOB
#define btnTapTempo_Pin GPIO_PIN_8
#define btnTapTempo_GPIO_Port GPIOB
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
