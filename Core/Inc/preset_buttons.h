/**
  ******************************************************************************
  * @file    preset_buttons.h
  * @author  Custom Implementation
  * @brief   Preset and Bank Button Handler
  ******************************************************************************
  */

#ifndef PRESET_BUTTONS_H
#define PRESET_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define NUM_BANKS           20
#define PRESETS_PER_BANK    5
#define MAX_PRESET_NUMBER   99

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize preset button handler
  * @retval None
  */
void PresetButtons_Init(void);

/**
  * @brief  GPIO EXTI callback for button interrupts
  * @param  GPIO_Pin: Pin number that triggered the interrupt
  * @retval None
  */
void PresetButtons_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/**
  * @brief  Process button debounce timing (call from main loop)
  * @retval None
  */
void PresetButtons_Process(void);

/**
  * @brief  Get current bank number
  * @retval Current bank (0-19)
  */
uint8_t PresetButtons_GetCurrentBank(void);

/**
  * @brief  Get current preset number
  * @retval Current preset (0-99)
  */
uint8_t PresetButtons_GetCurrentPreset(void);

/**
  * @brief  Set current preset number (called when GP-5 reports preset)
  * @param  preset: Preset number (0-99)
  * @retval None
  */
void PresetButtons_SetCurrentPreset(uint8_t preset);

/**
  * @brief  Notify that a preset change ACK was received
  * @param  sentByUs: true if we sent the command, false if external
  * @retval None
  */
void PresetButtons_OnPresetChangeACK(bool sentByUs);

#ifdef __cplusplus
}
#endif

#endif /* PRESET_BUTTONS_H */
