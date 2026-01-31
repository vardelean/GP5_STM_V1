/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/**
  * @brief  Configure GPIO pins for USB Host operation
  * @retval None
  */
void MX_GPIO_Init(void)
{
 GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PWR_GPIO_Port, USB_PWR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : btnPst0_Pin btnPst1_Pin btnPst2_Pin btnPst3_Pin
                           btnPst4_Pin btnBankUp_Pin btnBankDown_Pin btnCtl_Pin
                           btnTapTempo_Pin */
  GPIO_InitStruct.Pin = btnPst0_Pin|btnPst1_Pin|btnPst2_Pin|btnPst3_Pin
                          |btnPst4_Pin|btnBankUp_Pin|btnBankDown_Pin|btnCtl_Pin
                          |btnTapTempo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PWR_Pin */
  GPIO_InitStruct.Pin = USB_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PWR_GPIO_Port, &GPIO_InitStruct);
}
