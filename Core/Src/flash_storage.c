/**
  ******************************************************************************
  * @file    flash_storage.c
  * @author  Custom Implementation
  * @brief   Flash storage implementation for preset data
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_storage.h"
#include "stm32g0xx_hal_flash.h"
#include "stm32g0xx_hal_flash_ex.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t presetArray[NUM_PRESETS];        /* Working copy in RAM */
static uint8_t currentPresetIndex = 0;          /* Current STM32 preset index */
static bool isInitialized = false;

/* Private function prototypes -----------------------------------------------*/
static int ReadPresetsFromFlash(void);
static int WritePresetsToFlash(void);
static int ReadCurrentPresetIndex(void);
static int WriteCurrentPresetIndex(uint8_t index);
static bool CheckInitializationFlag(void);
static int SetInitializationFlag(void);
static int EraseFlashPage(uint32_t pageAddress);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize flash storage system
  * @retval 0 on success, -1 on error
  */
int FlashStorage_Init(void)
{
  printf("[Flash] Initializing storage system...\r\n");
  
  /* Check if Flash has been initialized before */
  bool alreadyInitialized = CheckInitializationFlag();
  
  if (!alreadyInitialized)
  {
    printf("[Flash] First initialization detected\r\n");
    
    /* Initialize preset array with empty values */
    memset(presetArray, PRESET_EMPTY, NUM_PRESETS);
    currentPresetIndex = 0;
    
    /* Write initial data to Flash */
    if (WritePresetsToFlash() != 0)
    {
      printf("[Flash] ERROR: Failed to write initial presets\r\n");
      return -1;
    }
    
    if (WriteCurrentPresetIndex(0) != 0)
    {
      printf("[Flash] ERROR: Failed to write initial preset index\r\n");
      return -1;
    }
    
    /* Set initialization flag */
    if (SetInitializationFlag() != 0)
    {
      printf("[Flash] ERROR: Failed to set initialization flag\r\n");
      return -1;
    }
    
    printf("[Flash] Initial data written successfully\r\n");
  }
  else
  {
    printf("[Flash] Loading existing data from Flash...\r\n");
    
    /* Load existing data from Flash to RAM */
    if (ReadPresetsFromFlash() != 0)
    {
      printf("[Flash] ERROR: Failed to read presets\r\n");
      return -1;
    }
    
    if (ReadCurrentPresetIndex() != 0)
    {
      printf("[Flash] ERROR: Failed to read current preset index\r\n");
      return -1;
    }
    
    printf("[Flash] Data loaded: Current index = %d\r\n", currentPresetIndex);
  }
  
  isInitialized = true;
  printf("[Flash] Storage system ready\r\n");
  return 0;
}

/**
  * @brief  Get preset array from RAM (working copy)
  * @retval Pointer to preset array (80 bytes)
  */
uint8_t* FlashStorage_GetPresetArray(void)
{
  return presetArray;
}

/**
  * @brief  Get stored GP-5 preset number for a given STM32 preset index
  * @param  presetIndex: STM32 preset index (0-79)
  * @retval GP-5 preset number (0-99) or PRESET_EMPTY (0xFF)
  */
uint8_t FlashStorage_GetPreset(uint8_t presetIndex)
{
  if (presetIndex >= NUM_PRESETS)
    return PRESET_EMPTY;
  
  return presetArray[presetIndex];
}

/**
  * @brief  Save GP-5 preset number to a STM32 preset location
  * @param  presetIndex: STM32 preset index (0-79)
  * @param  gp5Preset: GP-5 preset number (0-99) or PRESET_EMPTY (0xFF)
  * @retval 0 on success, -1 on error
  */
int FlashStorage_SavePreset(uint8_t presetIndex, uint8_t gp5Preset)
{
  if (presetIndex >= NUM_PRESETS)
  {
    printf("[Flash] ERROR: Invalid preset index: %d\r\n", presetIndex);
    return -1;
  }
  
  /* Validate GP-5 preset number */
  if (gp5Preset != PRESET_EMPTY && gp5Preset > PRESET_VALID_MAX)
  {
    printf("[Flash] ERROR: Invalid GP-5 preset: %d\r\n", gp5Preset);
    return -1;
  }
  
  /* Update RAM copy */
  presetArray[presetIndex] = gp5Preset;
  
  /* Write to Flash */
  if (WritePresetsToFlash() != 0)
  {
    printf("[Flash] ERROR: Failed to write preset to Flash\r\n");
    return -1;
  }
  
  printf("[Flash] Saved: STM32[%d] = GP-5[%d]\r\n", presetIndex, gp5Preset);
  return 0;
}

/**
  * @brief  Get current STM32 preset index
  * @retval Current preset index (0-79)
  */
uint8_t FlashStorage_GetCurrentPresetIndex(void)
{
  return currentPresetIndex;
}

/**
  * @brief  Save current STM32 preset index with wear leveling
  * @param  presetIndex: STM32 preset index to save (0-79)
  * @retval 0 on success, -1 on error
  */
int FlashStorage_SaveCurrentPresetIndex(uint8_t presetIndex)
{
  if (presetIndex >= NUM_PRESETS)
  {
    printf("[Flash] ERROR: Invalid preset index: %d\r\n", presetIndex);
    return -1;
  }
  
  currentPresetIndex = presetIndex;
  
  if (WriteCurrentPresetIndex(presetIndex) != 0)
  {
    printf("[Flash] ERROR: Failed to write current preset index\r\n");
    return -1;
  }
  
  return 0;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Read presets from Flash to RAM
  * @retval 0 on success, -1 on error
  */
static int ReadPresetsFromFlash(void)
{
  uint32_t flashAddress = PRESET_STORAGE_ADDRESS;
  
  /* Read array A (first 80 bytes) */
  uint8_t* flashPtr = (uint8_t*)flashAddress;
  memcpy(presetArray, flashPtr, NUM_PRESETS);
  
  /* Read array B (next 80 bytes) for verification */
  flashPtr += NUM_PRESETS;
  uint8_t arrayB[NUM_PRESETS];
  memcpy(arrayB, flashPtr, NUM_PRESETS);
  
  /* Compare arrays for data integrity */
  if (memcmp(presetArray, arrayB, NUM_PRESETS) != 0)
  {
    printf("[Flash] WARNING: Array A and B mismatch, using Array A\r\n");
  }
  
  return 0;
}

/**
  * @brief  Write presets from RAM to Flash (dual array)
  * @retval 0 on success, -1 on error
  */
static int WritePresetsToFlash(void)
{
  HAL_StatusTypeDef status;
  
  /* Unlock Flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    printf("[Flash] ERROR: Failed to unlock Flash\r\n");
    return -1;
  }
  
  /* Erase page */
  if (EraseFlashPage(PRESET_STORAGE_ADDRESS) != 0)
  {
    HAL_FLASH_Lock();
    return -1;
  }
  
  /* Write array A */
  uint32_t address = PRESET_STORAGE_ADDRESS;
  for (uint32_t i = 0; i < NUM_PRESETS; i += 8)
  {
    uint64_t data = 0;
    memcpy(&data, &presetArray[i], 8);
    
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    if (status != HAL_OK)
    {
      printf("[Flash] ERROR: Failed to write array A at offset %lu\r\n", i);
      HAL_FLASH_Lock();
      return -1;
    }
    address += 8;
  }
  
  /* Write array B (duplicate for redundancy) */
  for (uint32_t i = 0; i < NUM_PRESETS; i += 8)
  {
    uint64_t data = 0;
    memcpy(&data, &presetArray[i], 8);
    
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    if (status != HAL_OK)
    {
      printf("[Flash] ERROR: Failed to write array B at offset %lu\r\n", i);
      HAL_FLASH_Lock();
      return -1;
    }
    address += 8;
  }
  
  /* Lock Flash */
  HAL_FLASH_Lock();
  
  return 0;
}

/**
  * @brief  Read current preset index from Flash (with wear leveling)
  * @retval 0 on success, -1 on error
  */
static int ReadCurrentPresetIndex(void)
{
  uint32_t* flashPtr = (uint32_t*)CURRENT_PRESET_ADDRESS;
  
  /* Find the last written entry (wear leveling) */
  /* Format: [index (1 byte), checksum (1 byte), 0xFF, 0xFF, marker (4 bytes)] = 8 bytes */
  int lastValidEntry = -1;
  
  for (int i = 0; i < WEAR_LEVEL_ENTRIES; i++)
  {
    uint32_t word1 = flashPtr[i * 2];
    uint32_t word2 = flashPtr[i * 2 + 1];
    
    /* Check if entry is written (not 0xFFFFFFFF) */
    if (word1 != 0xFFFFFFFF && word2 == 0x5A5A5A5A)  /* Marker check */
    {
      uint8_t index = (uint8_t)(word1 & 0xFF);
      uint8_t checksum = (uint8_t)((word1 >> 8) & 0xFF);
      
      /* Verify checksum */
      if (checksum == (uint8_t)(~index))
      {
        lastValidEntry = i;
      }
    }
  }
  
  if (lastValidEntry >= 0)
  {
    uint32_t word1 = flashPtr[lastValidEntry * 2];
    currentPresetIndex = (uint8_t)(word1 & 0xFF);
    
    /* Validate range */
    if (currentPresetIndex >= NUM_PRESETS)
    {
      printf("[Flash] WARNING: Invalid preset index %d, resetting to 0\r\n", currentPresetIndex);
      currentPresetIndex = 0;
    }
  }
  else
  {
    /* No valid entry found, default to 0 */
    currentPresetIndex = 0;
  }
  
  return 0;
}

/**
  * @brief  Write current preset index to Flash (with wear leveling)
  * @param  index: Preset index to write (0-79)
  * @retval 0 on success, -1 on error
  */
static int WriteCurrentPresetIndex(uint8_t index)
{
  HAL_StatusTypeDef status;
  uint32_t* flashPtr = (uint32_t*)CURRENT_PRESET_ADDRESS;
  
  /* Find next available slot */
  int nextSlot = -1;
  for (int i = 0; i < WEAR_LEVEL_ENTRIES; i++)
  {
    if (flashPtr[i * 2] == 0xFFFFFFFF)
    {
      nextSlot = i;
      break;
    }
  }
  
  /* Unlock Flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    printf("[Flash] ERROR: Failed to unlock Flash for index write\r\n");
    return -1;
  }
  
  /* If no slot available, erase page and start over */
  if (nextSlot < 0)
  {
    printf("[Flash] Wear leveling: Erasing current preset page\r\n");
    if (EraseFlashPage(CURRENT_PRESET_ADDRESS) != 0)
    {
      HAL_FLASH_Lock();
      return -1;
    }
    nextSlot = 0;
  }
  
  /* Write entry: [index (1 byte), checksum (1 byte), 0xFF, 0xFF] [marker (0x5A5A5A5A)] */
  uint8_t checksum = (uint8_t)(~index);
  uint64_t data = ((uint64_t)0x5A5A5A5A << 32) | (0xFFFF0000) | (checksum << 8) | index;
  
  uint32_t address = CURRENT_PRESET_ADDRESS + (nextSlot * 8);
  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
  
  if (status != HAL_OK)
  {
    printf("[Flash] ERROR: Failed to write current preset index\r\n");
    HAL_FLASH_Lock();
    return -1;
  }
  
  /* Lock Flash */
  HAL_FLASH_Lock();
  
  return 0;
}

/**
  * @brief  Check if Flash has been initialized
  * @retval true if initialized, false otherwise
  */
static bool CheckInitializationFlag(void)
{
  uint32_t* flagPtr = (uint32_t*)INIT_FLAG_ADDRESS;
  return (*flagPtr == INIT_FLAG_VALUE);
}

/**
  * @brief  Set initialization flag in Flash
  * @retval 0 on success, -1 on error
  */
static int SetInitializationFlag(void)
{
  HAL_StatusTypeDef status;
  
  /* Unlock Flash */
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    printf("[Flash] ERROR: Failed to unlock Flash for init flag\r\n");
    return -1;
  }
  
  /* Erase page */
  if (EraseFlashPage(INIT_FLAG_ADDRESS) != 0)
  {
    HAL_FLASH_Lock();
    return -1;
  }
  
  /* Write flag */
  uint64_t data = ((uint64_t)INIT_FLAG_VALUE << 32) | INIT_FLAG_VALUE;
  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, INIT_FLAG_ADDRESS, data);
  
  if (status != HAL_OK)
  {
    printf("[Flash] ERROR: Failed to write init flag\r\n");
    HAL_FLASH_Lock();
    return -1;
  }
  
  /* Lock Flash */
  HAL_FLASH_Lock();
  
  return 0;
}

/**
  * @brief  Erase a Flash page
  * @param  pageAddress: Start address of the page
  * @retval 0 on success, -1 on error
  */
static int EraseFlashPage(uint32_t pageAddress)
{
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef eraseInit;
  uint32_t pageError = 0;
  
  /* Calculate page number */
  uint32_t pageNumber = (pageAddress - FLASH_START_ADDRESS) / FLASH_PAGE_SIZE;
  
  /* Configure erase */
  eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
  eraseInit.Page = pageNumber;
  eraseInit.NbPages = 1;
  
  /* Perform erase */
  status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
  
  if (status != HAL_OK)
  {
    printf("[Flash] ERROR: Failed to erase page %lu (error: 0x%lx)\r\n", pageNumber, pageError);
    return -1;
  }
  
  return 0;
}
