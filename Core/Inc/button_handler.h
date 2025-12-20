/**
  ******************************************************************************
  * @file    button_handler.h
  * @author  Custom Implementation
  * @brief   Header for button_handler.c file
  ******************************************************************************
  */

#ifndef __BUTTON_HANDLER_H
#define __BUTTON_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  BTN_UP = 0,
  BTN_DOWN,
  BTN_SCENE1,
  BTN_SCENE2,
  BTN_SCENE3,
  BTN_TAP,
  BTN_COUNT
} ButtonID_t;

typedef enum
{
  BTN_EVENT_NONE = 0,
  BTN_EVENT_SHORT_PRESS,
  BTN_EVENT_LONG_PRESS,
  BTN_EVENT_EXTRA_LONG_PRESS,
  BTN_EVENT_RELEASED
} ButtonEvent_t;

typedef struct
{
  ButtonID_t button;
  ButtonEvent_t event;
} ButtonEventData_t;

/* Callback function type */
typedef void (*ButtonEventCallback_t)(ButtonID_t button, ButtonEvent_t event);

/* Exported constants --------------------------------------------------------*/
#define BUTTON_DEBOUNCE_TIME_MS     50
#define BUTTON_LONG_PRESS_TIME_MS   2000
#define BUTTON_EXTRA_LONG_PRESS_TIME_MS   5000

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void ButtonHandler_Init(void);
void ButtonHandler_RegisterCallback(ButtonEventCallback_t callback);
void ButtonHandler_Process(void);
void ButtonHandler_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* Internal use - called from SysTick */
void ButtonHandler_TimerUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_HANDLER_H */
