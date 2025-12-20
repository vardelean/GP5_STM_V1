/**
  ******************************************************************************
  * @file    flash_storage.c
  * @author  Custom Implementation
  * @brief   Flash memory storage for button configuration data
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_storage.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize flash storage
  * @retval Flash storage status
  */
FlashStorage_StatusTypeDef FlashStorage_Init(void)
{
  /* Nothing to initialize for flash - ready to use */
  return FLASH_STORAGE_OK;
}

/**
  * @brief  Write 4-byte data to flash at specified index
  * @param  index: Storage index (0 to FLASH_STORAGE_MAX_ENTRIES-1)
  * @param  data: Pointer to 4-byte data array
  * @retval Flash storage status
  */
FlashStorage_StatusTypeDef FlashStorage_WriteData(uint16_t index, uint8_t *data)
{
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t page_error;
  uint8_t temp_buffer[FLASH_STORAGE_PAGE_SIZE];
  
  if (index >= FLASH_STORAGE_MAX_ENTRIES || data == NULL)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  /* Read entire page into temp buffer */
  memcpy(temp_buffer, (uint8_t *)FLASH_STORAGE_BASE_ADDRESS, FLASH_STORAGE_PAGE_SIZE);
  
  /* Update data at the specified index */
  memcpy(&temp_buffer[index * FLASH_STORAGE_DATA_SIZE], data, FLASH_STORAGE_DATA_SIZE);
  
  /* Unlock flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  /* Erase the page */
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Page = 255;  /* Last page */
  EraseInitStruct.NbPages = 1;
  
  status = HAL_FLASHEx_Erase(&EraseInitStruct, &page_error);
  if (status != HAL_OK)
  {
    HAL_FLASH_Lock();
    return FLASH_STORAGE_ERROR;
  }
  
  /* Write temp buffer back to flash (64-bit aligned writes) */
  for (uint32_t i = 0; i < FLASH_STORAGE_PAGE_SIZE; i += 8)
  {
    uint64_t data_64bit = 0;
    memcpy(&data_64bit, &temp_buffer[i], 8);
    
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 
                               FLASH_STORAGE_BASE_ADDRESS + i, 
                               data_64bit);
    
    if (status != HAL_OK)
    {
      HAL_FLASH_Lock();
      return FLASH_STORAGE_ERROR;
    }
  }
  
  /* Lock flash */
  HAL_FLASH_Lock();
  
  return FLASH_STORAGE_OK;
}

/**
  * @brief  Read 4-byte data from flash at specified index
  * @param  index: Storage index (0 to FLASH_STORAGE_MAX_ENTRIES-1)
  * @param  data: Pointer to 4-byte buffer to store read data
  * @retval Flash storage status
  */
FlashStorage_StatusTypeDef FlashStorage_ReadData(uint16_t index, uint8_t *data)
{
  uint32_t address;
  
  if (index >= FLASH_STORAGE_MAX_ENTRIES || data == NULL)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  /* Calculate address for this index */
  address = FLASH_STORAGE_BASE_ADDRESS + (index * FLASH_STORAGE_DATA_SIZE);
  
  /* Read data */
  memcpy(data, (uint8_t *)address, FLASH_STORAGE_DATA_SIZE);
  
  return FLASH_STORAGE_OK;
}

/**
  * @brief  Erase all stored data
  * @retval Flash storage status
  */
FlashStorage_StatusTypeDef FlashStorage_EraseAll(void)
{
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t page_error;
  
  /* Unlock flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  /* Erase the page */
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Page = 255;  /* Last page */
  EraseInitStruct.NbPages = 1;
  
  status = HAL_FLASHEx_Erase(&EraseInitStruct, &page_error);
  
  /* Lock flash */
  HAL_FLASH_Lock();
  
  if (status != HAL_OK)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  return FLASH_STORAGE_OK;
}

/**
  * @brief  Read entire scene database from flash to RAM
  * @param  data: Pointer to buffer to receive scene data
  * @param  size: Size of data to read (should be 1500 bytes)
  * @retval FlashStorage_StatusTypeDef
  */
FlashStorage_StatusTypeDef FlashStorage_ReadScenes(uint8_t *data, uint16_t size)
{
  if (data == NULL || size > FLASH_SCENE_SIZE)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  uint32_t address = FLASH_SCENE_BASE_ADDRESS;
  
  /* Read data from flash */
  for (uint16_t i = 0; i < size; i++)
  {
    data[i] = *(uint8_t*)(address + i);
  }
  
  return FLASH_STORAGE_OK;
}

/**
  * @brief  Write preset scenes to flash (10 bytes per preset - Scene 2 & 3 only)
  * @param  preset: Preset number (0-99)
  * @param  data: Pointer to PresetScenes_t structure (10 bytes)
  * @retval FlashStorage_StatusTypeDef
  */
FlashStorage_StatusTypeDef FlashStorage_WritePresetScenes(uint8_t preset, void *data)
{
  if (data == NULL || preset >= 100)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t page_error;
  
  /* Calculate address for this preset (10 bytes per preset) */
  uint32_t preset_address = FLASH_SCENE_BASE_ADDRESS + (preset * 10);
  
  /* Unlock flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  /* Read entire page into RAM */
  uint8_t page_buffer[FLASH_STORAGE_PAGE_SIZE];
  uint32_t page_address = FLASH_SCENE_BASE_ADDRESS;
  
  for (uint16_t i = 0; i < FLASH_STORAGE_PAGE_SIZE; i++)
  {
    page_buffer[i] = *(uint8_t*)(page_address + i);
  }
  
  /* Modify the 10 bytes for this preset in the buffer */
  uint16_t offset = preset * 10;
  memcpy(&page_buffer[offset], data, 10);
  
  /* Erase the page (page 254) */
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Page = 254;  /* Scene database page */
  EraseInitStruct.NbPages = 1;
  
  status = HAL_FLASHEx_Erase(&EraseInitStruct, &page_error);
  
  if (status != HAL_OK)
  {
    HAL_FLASH_Lock();
    return FLASH_STORAGE_ERROR;
  }
  
  /* Write entire page back with modified data */
  for (uint16_t i = 0; i < FLASH_STORAGE_PAGE_SIZE; i += 8)
  {
    uint64_t data_to_write = *(uint64_t*)&page_buffer[i];
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, page_address + i, data_to_write);
    
    if (status != HAL_OK)
    {
      HAL_FLASH_Lock();
      return FLASH_STORAGE_ERROR;
    }
  }
  
  /* Lock flash */
  HAL_FLASH_Lock();
  
  return FLASH_STORAGE_OK;
}

/**
  * @brief  Erase entire scene database (restore to factory defaults)
  * @retval FlashStorage_StatusTypeDef
  */
FlashStorage_StatusTypeDef FlashStorage_EraseSceneDatabase(void)
{
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t page_error;
  
  /* Unlock flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  /* Erase page 254 (scene database) */
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Page = 254;
  EraseInitStruct.NbPages = 1;
  
  status = HAL_FLASHEx_Erase(&EraseInitStruct, &page_error);
  
  /* Lock flash */
  HAL_FLASH_Lock();
  
  if (status != HAL_OK)
  {
    return FLASH_STORAGE_ERROR;
  }
  
  return FLASH_STORAGE_OK;
}
