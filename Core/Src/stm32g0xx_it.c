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
#include "preset_buttons.h"
#include <stdio.h>
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
extern HCD_HandleTypeDef hhcd_USB_DRD_FS;
extern I2C_HandleTypeDef hi2c2;
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
  /* Check btnPst0 (PB0) and btnPst1 (PB1) */
  if (__HAL_GPIO_EXTI_GET_IT(btnPst0_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnPst0_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnPst0_Pin);
  }
  
  if (__HAL_GPIO_EXTI_GET_IT(btnPst1_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnPst1_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnPst1_Pin);
  }
}

/**
  * @brief This function handles EXTI line 2 and 3 interrupts.
  */
void EXTI2_3_IRQHandler(void)
{
  /* Check btnPst2 (PB2) and btnPst3 (PB3) */
  if (__HAL_GPIO_EXTI_GET_IT(btnPst2_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnPst2_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnPst2_Pin);
  }
  
  if (__HAL_GPIO_EXTI_GET_IT(btnPst3_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnPst3_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnPst3_Pin);
  }
}

/**
  * @brief This function handles EXTI line 4 to 15 interrupts.
  */
void EXTI4_15_IRQHandler(void)
{
  /* Check btnPst4 (PB4) */
  if (__HAL_GPIO_EXTI_GET_IT(btnPst4_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnPst4_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnPst4_Pin);
  }
  
  /* Check btnBankUp (PB5) */
  if (__HAL_GPIO_EXTI_GET_IT(btnBankUp_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnBankUp_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnBankUp_Pin);
  }
  
  /* Check btnBankDown (PB6) */
  if (__HAL_GPIO_EXTI_GET_IT(btnBankDown_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnBankDown_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnBankDown_Pin);
  }
  
  /* Check btnCtl (PB7) */
  if (__HAL_GPIO_EXTI_GET_IT(btnCtl_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnCtl_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnCtl_Pin);
  }
  
  /* Check btnTapTempo (PB8) */
  if (__HAL_GPIO_EXTI_GET_IT(btnTapTempo_Pin) != 0)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(btnTapTempo_Pin);
    PresetButtons_GPIO_EXTI_Callback(btnTapTempo_Pin);
  }
}


/**
  * @brief This function handles I2C2, I2C3 Interrupt (combined with EXTI 24 and EXTI 22).
  */
void I2C2_3_IRQHandler(void)
{
 
  if (hi2c2.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR))
  {
    HAL_I2C_ER_IRQHandler(&hi2c2);
  }
  else
  {
    HAL_I2C_EV_IRQHandler(&hi2c2);
  }
}
