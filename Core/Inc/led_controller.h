/**
  ******************************************************************************
  * @file    led_controller.h
  * @author  Custom Implementation
  * @brief   Header for led_controller.c file
  ******************************************************************************
  */

#ifndef __LED_CONTROLLER_H
#define __LED_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  LED_SCENE1 = 0,
  LED_SCENE2,
  LED_SCENE3,
  LED_COUNT
} LED_ID_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void LED_Init(void);
void LED_SetScene(LED_ID_t led);
void LED_TurnOn(LED_ID_t led);
void LED_TurnOff(LED_ID_t led);
void LED_Toggle(LED_ID_t led);
void LED_AllOff(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONTROLLER_H */
