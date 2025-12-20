/**
  ******************************************************************************
  * @file    stm32g0xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32g0xx_it.h"
/* Private includes ----------------------------------------------------------*/
#include "button_handler.h"
#include <stdio.h>
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern HCD_HandleTypeDef hhcd_USB_DRD_FS;
/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
   while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  ButtonHandler_TimerUpdate();
  HAL_IncTick();
}

/******************************************************************************/
/* STM32G0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g0xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USB, UCPD1 and UCPD2 global interrupts.
  */
void USB_UCPD1_2_IRQHandler(void)
{
  HAL_HCD_IRQHandler(&hhcd_USB_DRD_FS);
}
/**
  * @brief This function handles EXTI line 0 and 1 interrupts.
  */
void EXTI0_1_IRQHandler(void)
{
  uint32_t pr = EXTI->RPR1 | EXTI->FPR1;

  
  /* Manually check and clear pending bits, then call button handler */
  if (pr & btnScene1_Pin) {
    __HAL_GPIO_EXTI_CLEAR_IT(btnScene1_Pin);
    ButtonHandler_GPIO_EXTI_Callback(btnScene1_Pin);
  }
}

/**
  * @brief This function handles EXTI line 2 and 3 interrupts.
  */
void EXTI2_3_IRQHandler(void)
{
  uint32_t pr = EXTI->RPR1 | EXTI->FPR1;

  
  /* Manually check and clear pending bits, then call button handler */
  if (pr & btnScene2_Pin) {
    __HAL_GPIO_EXTI_CLEAR_IT(btnScene2_Pin);
    ButtonHandler_GPIO_EXTI_Callback(btnScene2_Pin);
  }
  if (pr & btnScene3_Pin) {
    __HAL_GPIO_EXTI_CLEAR_IT(btnScene3_Pin);
    ButtonHandler_GPIO_EXTI_Callback(btnScene3_Pin);
  }
}

/**
  * @brief This function handles EXTI line 4 to 15 interrupts.
  */
void EXTI4_15_IRQHandler(void)
{
  uint32_t pr = EXTI->RPR1 | EXTI->FPR1;
  
  /* Manually check and clear pending bits, then call button handler */
  if (pr & btnUp_Pin) {
    __HAL_GPIO_EXTI_CLEAR_IT(btnUp_Pin);
    ButtonHandler_GPIO_EXTI_Callback(btnUp_Pin);
  }
  if (pr & btnDown_Pin) {
    __HAL_GPIO_EXTI_CLEAR_IT(btnDown_Pin);
    ButtonHandler_GPIO_EXTI_Callback(btnDown_Pin);
  }
  if (pr & btnTap_Pin) {
    __HAL_GPIO_EXTI_CLEAR_IT(btnTap_Pin);
    ButtonHandler_GPIO_EXTI_Callback(btnTap_Pin);
  }
}

/**
  * @brief GPIO EXTI callback
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  printf("[EXTI] HAL Callback triggered, pin=0x%04X\r\n", GPIO_Pin);
  ButtonHandler_GPIO_EXTI_Callback(GPIO_Pin);
}
