/**
  ******************************************************************************
  * @file    button_handler.c
  * @author  Custom Implementation
  * @brief   Button handling with interrupt, debounce, and long-press detection
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "button_handler.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  uint8_t pressed;
  uint8_t debounce_active;
  uint32_t press_start_time;
  uint32_t debounce_start_time;
  uint8_t long_press_detected;
  uint8_t extra_long_press_detected;
  uint8_t event_pending;
  ButtonEvent_t pending_event;
} ButtonState_t;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ButtonState_t button_states[BTN_COUNT];
static ButtonEventCallback_t event_callback = NULL;
static uint8_t active_button_index = 0xFF;  /* 0xFF means no button active */
static uint32_t system_tick_ms = 0;

/* Button pin mapping */
static const struct {
  GPIO_TypeDef *port;
  uint16_t pin;
} button_pins[BTN_COUNT] = {
  {btnUp_GPIO_Port, btnUp_Pin},           /* BTN_UP */
  {btnDown_GPIO_Port, btnDown_Pin},       /* BTN_DOWN */
  {btnScene1_GPIO_Port, btnScene1_Pin},   /* BTN_SCENE1 */
  {btnScene2_GPIO_Port, btnScene2_Pin},   /* BTN_SCENE2 */
  {btnScene3_GPIO_Port, btnScene3_Pin},   /* BTN_SCENE3 */
  {btnTap_GPIO_Port, btnTap_Pin}          /* BTN_TAP */
};

/* Private function prototypes -----------------------------------------------*/
static ButtonID_t GetButtonIDFromPin(uint16_t GPIO_Pin);
static void ProcessButtonPress(ButtonID_t button_id);
static void ProcessButtonRelease(ButtonID_t button_id);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize button handler
  * @retval None
  */
void ButtonHandler_Init(void)
{
  /* Initialize button states */
  memset(button_states, 0, sizeof(button_states));
  
  for (uint8_t i = 0; i < BTN_COUNT; i++)
  {
    button_states[i].port = button_pins[i].port;
    button_states[i].pin = button_pins[i].pin;
  }
  
  active_button_index = 0xFF;
  system_tick_ms = 0;
  
  /* Enable EXTI interrupts for button pins */
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
  
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
  
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

/**
  * @brief  Register callback for button events
  * @param  callback: Callback function pointer
  * @retval None
  */
void ButtonHandler_RegisterCallback(ButtonEventCallback_t callback)
{
  event_callback = callback;
}

/**
  * @brief  Update timer tick (call from SysTick_Handler)
  * @retval None
  */
void ButtonHandler_TimerUpdate(void)
{
  system_tick_ms++;
}

/**
  * @brief  Process button states (call from main loop)
  * @retval None
  */
void ButtonHandler_Process(void)
{
  /* Check for long press and extra long press */
  if (active_button_index != 0xFF)
  {
    ButtonState_t *btn = &button_states[active_button_index];
    
    if (btn->pressed)
    {
      uint32_t press_duration = system_tick_ms - btn->press_start_time;
      
      /* Check for extra long press (5 seconds) */
      if (!btn->extra_long_press_detected && press_duration >= BUTTON_EXTRA_LONG_PRESS_TIME_MS)
      {
        btn->extra_long_press_detected = 1;
        btn->event_pending = 1;
        btn->pending_event = BTN_EVENT_EXTRA_LONG_PRESS;
      }
      /* Check for long press (1 second) */
      else if (!btn->long_press_detected && press_duration >= BUTTON_LONG_PRESS_TIME_MS)
      {
        btn->long_press_detected = 1;
        btn->event_pending = 1;
        btn->pending_event = BTN_EVENT_LONG_PRESS;
      }
    }
  }
  
  /* Process pending events */
  for (uint8_t i = 0; i < BTN_COUNT; i++)
  {
    if (button_states[i].event_pending && event_callback != NULL)
    {
      event_callback((ButtonID_t)i, button_states[i].pending_event);
      button_states[i].event_pending = 0;
    }
  }
  
  /* Handle debouncing */
  for (uint8_t i = 0; i < BTN_COUNT; i++)
  {
    if (button_states[i].debounce_active)
    {
      uint32_t debounce_time = system_tick_ms - button_states[i].debounce_start_time;
      
      if (debounce_time >= BUTTON_DEBOUNCE_TIME_MS)
      {
        button_states[i].debounce_active = 0;
        
        /* Re-enable interrupt for this button */
        __HAL_GPIO_EXTI_CLEAR_IT(button_states[i].pin);
      }
    }
  }
}

/**
  * @brief  GPIO EXTI callback (called from HAL_GPIO_EXTI_Callback)
  * @param  GPIO_Pin: Pin number
  * @retval None
  */
void ButtonHandler_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  ButtonID_t button_id = GetButtonIDFromPin(GPIO_Pin);
  
  if (button_id < BTN_COUNT)
  {
    ButtonState_t *btn = &button_states[button_id];
    
    /* Check if debounce is active */
    if (btn->debounce_active)
    {
      return;
    }
    
    /* Read current pin state */
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(btn->port, btn->pin);
    
    if (pin_state == GPIO_PIN_RESET)  /* Button pressed (active low) */
    {
      /* Only allow one button at a time */
      if (active_button_index == 0xFF)
      {
        ProcessButtonPress(button_id);
      }
    }
    else  /* Button released */
    {
      /* Only process release for the currently active button */
      if (active_button_index == button_id)
      {
        ProcessButtonRelease(button_id);
      }
    }
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Get button ID from GPIO pin
  * @param  GPIO_Pin: Pin number
  * @retval Button ID
  */
static ButtonID_t GetButtonIDFromPin(uint16_t GPIO_Pin)
{
  for (uint8_t i = 0; i < BTN_COUNT; i++)
  {
    if (button_pins[i].pin == GPIO_Pin)
    {
      return (ButtonID_t)i;
    }
  }
  return BTN_COUNT;  /* Invalid */
}

/**
  * @brief  Process button press
  * @param  button_id: Button ID
  * @retval None
  */
static void ProcessButtonPress(ButtonID_t button_id)
{
  ButtonState_t *btn = &button_states[button_id];
  
  btn->pressed = 1;
  btn->press_start_time = system_tick_ms;
  btn->long_press_detected = 0;
  btn->extra_long_press_detected = 0;
  btn->debounce_active = 1;
  btn->debounce_start_time = system_tick_ms;
  
  active_button_index = button_id;
}

/**
  * @brief  Process button release
  * @param  button_id: Button ID
  * @retval None
  */
static void ProcessButtonRelease(ButtonID_t button_id)
{
  ButtonState_t *btn = &button_states[button_id];
  
  if (btn->pressed)
  {
    btn->pressed = 0;
    
    /* Generate event based on press duration */
    if (!btn->long_press_detected)
    {
      /* Short press */
      btn->event_pending = 1;
      btn->pending_event = BTN_EVENT_SHORT_PRESS;
    }
    
    /* CRITICAL: Reset long press flags to prevent false triggers on rapid presses */
    btn->long_press_detected = 0;
    btn->extra_long_press_detected = 0;
    
    /* Release event (no callback for released) */
    
    /* Clear active button */
    active_button_index = 0xFF;
    
    /* Start debounce */
    btn->debounce_active = 1;
    btn->debounce_start_time = system_tick_ms;
  }
}
