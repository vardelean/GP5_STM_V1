/**
  ******************************************************************************
  * @file    flash_storage.h
  * @author  Custom Implementation
  * @brief   Flash storage for preset data with wear leveling
  ******************************************************************************
  */

#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32g0xx_hal.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define NUM_PRESETS             80      /* 16 banks x 5 presets */
#define PRESET_EMPTY            0xFF    /* Empty preset value */
#define PRESET_VALID_MAX        99      /* Maximum valid GP-5 preset number */

/* Flash page configuration (STM32G0B1: 2KB pages, 512KB total) */
/* Note: FLASH_PAGE_SIZE is already defined in stm32g0xx_hal_flash.h */
#define PRESET_DATA_PAGES       2       /* Use 2 pages for preset storage */
#define FLASH_START_ADDRESS     0x08000000
#define FLASH_TOTAL_SIZE        (512 * 1024)

/* Use last 4 pages of Flash for preset storage (pages 252-255) */
/* Page 252-253: Preset array A and B (dual array with redundancy) */
/* Page 254: Current preset index with wear leveling */
/* Page 255: Initialization flag */
#define PRESET_STORAGE_PAGE     252
#define PRESET_STORAGE_ADDRESS  (FLASH_START_ADDRESS + (PRESET_STORAGE_PAGE * FLASH_PAGE_SIZE))

#define CURRENT_PRESET_PAGE     254
#define CURRENT_PRESET_ADDRESS  (FLASH_START_ADDRESS + (CURRENT_PRESET_PAGE * FLASH_PAGE_SIZE))

#define INIT_FLAG_PAGE          255
#define INIT_FLAG_ADDRESS       (FLASH_START_ADDRESS + (INIT_FLAG_PAGE * FLASH_PAGE_SIZE))
#define INIT_FLAG_VALUE         0xA5A5A5A5  /* Magic number indicating initialization */

/* Wear leveling for current preset index */
#define WEAR_LEVEL_ENTRIES      (FLASH_PAGE_SIZE / 8)  /* Each entry is 8 bytes */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize flash storage system
  * @retval 0 on success, -1 on error
  */
int FlashStorage_Init(void);

/**
  * @brief  Get preset array from RAM (working copy)
  * @retval Pointer to preset array (80 bytes)
  */
uint8_t* FlashStorage_GetPresetArray(void);

/**
  * @brief  Get stored GP-5 preset number for a given STM32 preset index
  * @param  presetIndex: STM32 preset index (0-79)
  * @retval GP-5 preset number (0-99) or PRESET_EMPTY (0xFF)
  */
uint8_t FlashStorage_GetPreset(uint8_t presetIndex);

/**
  * @brief  Save GP-5 preset number to a STM32 preset location
  * @param  presetIndex: STM32 preset index (0-79)
  * @param  gp5Preset: GP-5 preset number (0-99) or PRESET_EMPTY (0xFF)
  * @retval 0 on success, -1 on error
  */
int FlashStorage_SavePreset(uint8_t presetIndex, uint8_t gp5Preset);

/**
  * @brief  Get current STM32 preset index
  * @retval Current preset index (0-79)
  */
uint8_t FlashStorage_GetCurrentPresetIndex(void);

/**
  * @brief  Save current STM32 preset index with wear leveling
  * @param  presetIndex: STM32 preset index to save (0-79)
  * @retval 0 on success, -1 on error
  */
int FlashStorage_SaveCurrentPresetIndex(uint8_t presetIndex);

/**
  * @brief  Calculate STM32 preset index from bank and button number
  * @param  bank: Bank number (0-15)
  * @param  button: Button number (0-4)
  * @retval Preset index (0-79)
  */
static inline uint8_t FlashStorage_CalculateIndex(uint8_t bank, uint8_t button)
{
  return (bank * 5) + button;
}

/**
  * @brief  Extract bank and button from preset index
  * @param  presetIndex: STM32 preset index (0-79)
  * @param  bank: Pointer to store bank number (0-15)
  * @param  button: Pointer to store button number (0-4)
  */
static inline void FlashStorage_ExtractBankButton(uint8_t presetIndex, uint8_t* bank, uint8_t* button)
{
  *bank = presetIndex / 5;
  *button = presetIndex % 5;
}

#ifdef __cplusplus
}
#endif

#endif /* FLASH_STORAGE_H */
