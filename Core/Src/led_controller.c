/**
  ******************************************************************************
  * @file    led_controller.c
  * @author  Custom Implementation
  * @brief   LED control with exclusive scene selection
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "led_controller.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const struct {
  GPIO_TypeDef *port;
  uint16_t pin;
} led_pins[LED_COUNT] = {
  {ledScene1_GPIO_Port, ledScene1_Pin},
  {ledScene2_GPIO_Port, ledScene2_Pin},
  {ledScene3_GPIO_Port, ledScene3_Pin}
};

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize LED controller
  * @retval None
  */
void LED_Init(void)
{
  /* Turn off all LEDs */
  LED_AllOff();
}

/**
  * @brief  Set scene LED (exclusive - turns off others)
  * @param  led: LED to turn on
  * @retval None
  */
void LED_SetScene(LED_ID_t led)
{
  /* Turn off all LEDs first */
  for (uint8_t i = 0; i < LED_COUNT; i++)
  {
    HAL_GPIO_WritePin(led_pins[i].port, led_pins[i].pin, GPIO_PIN_SET);
  }
  
  /* Turn on selected LED */
  if (led < LED_COUNT)
  {
    HAL_GPIO_WritePin(led_pins[led].port, led_pins[led].pin, GPIO_PIN_RESET);
  }
}

/**
  * @brief  Turn on LED
  * @param  led: LED to turn on
  * @retval None
  */
void LED_TurnOn(LED_ID_t led)
{
  if (led < LED_COUNT)
  {
    HAL_GPIO_WritePin(led_pins[led].port, led_pins[led].pin, GPIO_PIN_RESET);
  }
}

/**
  * @brief  Turn off LED
  * @param  led: LED to turn off
  * @retval None
  */
void LED_TurnOff(LED_ID_t led)
{
  if (led < LED_COUNT)
  {
    HAL_GPIO_WritePin(led_pins[led].port, led_pins[led].pin, GPIO_PIN_SET);
  }
}

/**
  * @brief  Toggle LED
  * @param  led: LED to toggle
  * @retval None
  */
void LED_Toggle(LED_ID_t led)
{
  if (led < LED_COUNT)
  {
    HAL_GPIO_TogglePin(led_pins[led].port, led_pins[led].pin);
  }
}

/**
  * @brief  Turn off all LEDs
  * @retval None
  */
void LED_AllOff(void)
{
  for (uint8_t i = 0; i < LED_COUNT; i++)
  {
    HAL_GPIO_WritePin(led_pins[i].port, led_pins[i].pin, GPIO_PIN_SET);
  }
}
