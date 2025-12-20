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
/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PWR_GPIO_Port, USB_PWR_Pin, GPIO_PIN_SET);  /* Enable USB Power */

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ledScene1_Pin|ledScene2_Pin|ledScene3_Pin|LED_GREEN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : B1_Pin btnScene1_Pin btnScene2_Pin btnScene3_Pin
                           btnUp_Pin btnDown_Pin btnTap_Pin */
  GPIO_InitStruct.Pin = B1_Pin|btnScene1_Pin|btnScene2_Pin|btnScene3_Pin
                          |btnUp_Pin|btnDown_Pin|btnTap_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;  /* Both edges for press AND release detection */
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PWR_Pin */
  GPIO_InitStruct.Pin = USB_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PWR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ledScene1_Pin ledScene2_Pin ledScene3_Pin */
  GPIO_InitStruct.Pin = ledScene1_Pin|ledScene2_Pin|ledScene3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

}
