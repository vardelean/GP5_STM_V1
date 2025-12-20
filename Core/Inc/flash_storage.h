/**
  ******************************************************************************
  * @file    flash_storage.h
  * @author  Custom Implementation
  * @brief   Header for flash_storage.c file
  ******************************************************************************
  */

#ifndef __FLASH_STORAGE_H
#define __FLASH_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  FLASH_STORAGE_OK = 0,
  FLASH_STORAGE_ERROR,
  FLASH_STORAGE_BUSY
} FlashStorage_StatusTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Use last page of flash for storage (STM32G0B1RE has 512KB flash = 256 pages of 2KB each) */
#define FLASH_STORAGE_BASE_ADDRESS    0x0807F800  /* Last page (page 255) */
#define FLASH_STORAGE_PAGE_SIZE       2048
#define FLASH_STORAGE_DATA_SIZE       4           /* 4 bytes per entry */
#define FLASH_STORAGE_MAX_ENTRIES     64          /* Maximum number of 4-byte entries */

/* Scene database storage - use page 254 (1000 bytes for 100 presets × 10 bytes) */
#define FLASH_SCENE_BASE_ADDRESS      0x0807F000  /* Page 254 */
#define FLASH_SCENE_SIZE              1000        /* 100 presets × 10 bytes (Scene 2 & 3 only) */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
FlashStorage_StatusTypeDef FlashStorage_Init(void);
FlashStorage_StatusTypeDef FlashStorage_WriteData(uint16_t index, uint8_t *data);
FlashStorage_StatusTypeDef FlashStorage_ReadData(uint16_t index, uint8_t *data);
FlashStorage_StatusTypeDef FlashStorage_EraseAll(void);

/* Scene database functions */
FlashStorage_StatusTypeDef FlashStorage_ReadScenes(uint8_t *data, uint16_t size);
FlashStorage_StatusTypeDef FlashStorage_WritePresetScenes(uint8_t preset, void *data);
FlashStorage_StatusTypeDef FlashStorage_EraseSceneDatabase(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_STORAGE_H */
