/**
  ******************************************************************************
  * @file    flash_test.c
  * @brief   Standalone Flash erase/write test
  ******************************************************************************
  */

#include "main.h"
#include "stm32g0xx_hal.h"
#include <stdio.h>

/* Test address - use page 59 (0x0801D800) instead of page 60 */
#define TEST_PAGE           59
#define TEST_ADDRESS        (0x08000000 + (TEST_PAGE * 0x800))

extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);
extern void MX_USART2_UART_Init(void);

/**
  * @brief  Simple Flash erase and write test
  */
void FlashTest_Run(void)
{
  HAL_StatusTypeDef status;
  
  printf("\r\n");
  printf("========================================\r\n");
  printf("    STM32G0B1 Flash Test Utility\r\n");
  printf("========================================\r\n");
  printf("Test Address: 0x%08lX (Page %d)\r\n", TEST_ADDRESS, TEST_PAGE);
  printf("\r\n");
  
  /* Check Flash option bytes and protection */
  printf("[0] Checking Flash protection...\r\n");
  FLASH_OBProgramInitTypeDef optionBytes;
  HAL_FLASHEx_OBGetConfig(&optionBytes);
  printf("    RDP Level: 0x%02lX\r\n", optionBytes.RDPLevel);
  printf("    WRP Area: 0x%08lX - 0x%08lX\r\n", 
         optionBytes.WRPStartOffset, optionBytes.WRPEndOffset);
  printf("    OPTR: 0x%08lX\r\n", FLASH->OPTR);
  printf("    PCROP1ASR: 0x%08lX\r\n", FLASH->PCROP1ASR);
  printf("    PCROP1AER: 0x%08lX\r\n", FLASH->PCROP1AER);
  printf("    WRP1AR: 0x%08lX\r\n", FLASH->WRP1AR);
  printf("    WRP1BR: 0x%08lX\r\n", FLASH->WRP1BR);
  
  /* Decode WRP1AR - bits 0-6 are start page, bits 16-22 are end page */
  uint32_t wrpStart = FLASH->WRP1AR & 0x7F;
  uint32_t wrpEnd = (FLASH->WRP1AR >> 16) & 0x7F;
  printf("    WRP protected pages: %lu to %lu\r\n", wrpStart, wrpEnd);
  
  if (TEST_PAGE >= wrpStart && TEST_PAGE <= wrpEnd)
  {
    printf("    ERROR: Page %d IS WRITE-PROTECTED!\r\n", TEST_PAGE);
    printf("    Need to disable write protection for pages 59-63\r\n");
    printf("    Use STM32CubeProgrammer to modify option bytes:\r\n");
    printf("      WRP1AR_STRT = 0x00 (start at page 0)\r\n");
    printf("      WRP1AR_END = 0x39 (end at page 57, before our data area)\r\n");
    printf("    Or set WRP1AR = 0xFF (no protection)\r\n");
    printf("\r\n");
    printf("    HALTING - Fix option bytes and re-run test\r\n");
    while(1);
  }
  
  printf("\r\n");
  
  /* Step 1: Read initial data */
  printf("[1] Reading initial data...\r\n");
  uint64_t* flashPtr = (uint64_t*)TEST_ADDRESS;
  printf("    Data: 0x%08lX%08lX\r\n", 
         (uint32_t)(*flashPtr >> 32), (uint32_t)(*flashPtr & 0xFFFFFFFF));
  
  /* Step 2: Unlock Flash */
  printf("[2] Unlocking Flash...\r\n");
  status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    printf("    ERROR: Unlock failed (status=%d)\r\n", status);
    while(1);
  }
  printf("    CR: 0x%08lX, SR: 0x%08lX\r\n", FLASH->CR, FLASH->SR);
  
  /* Step 3: Clear error flags */
  printf("[3] Clearing error flags and CR register...\r\n");
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR | 
                         FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | 
                         FLASH_FLAG_PGSERR);
  
  /* CRITICAL: Force CR to clean state (HAL preserves unknown bits) */
  FLASH->CR = 0x00000000;
  HAL_Delay(1);
  printf("    CR after forced clear: 0x%08lX\r\n", FLASH->CR);
  
  /* Step 4: Manual erase sequence (bypass HAL to control CR properly) */
  printf("[4] Manual Flash erase (page %d)...\r\n", TEST_PAGE);
  printf("    CR before: 0x%08lX\r\n", FLASH->CR);
  
  /* Clear CR completely except LOCK bit which should be 0 (unlocked) */
  FLASH->CR = 0x00000000;
  printf("    CR after clear: 0x%08lX\r\n", FLASH->CR);
  
  /* Set up erase: PER bit + page number */
  uint32_t cr_value = FLASH_CR_PER | (TEST_PAGE << FLASH_CR_PNB_Pos);
  FLASH->CR = cr_value;
  printf("    CR with PER+PNB: 0x%08lX\r\n", FLASH->CR);
  
  /* Start erase */
  FLASH->CR |= FLASH_CR_STRT;
  printf("    CR after STRT: 0x%08lX\r\n", FLASH->CR);
  
  /* Wait for BSY flag with timeout */
  uint32_t timeout = 1000;
  while ((FLASH->SR & FLASH_SR_BSY1) && timeout > 0)
  {
    HAL_Delay(1);
    timeout--;
  }
  
  if (timeout == 0)
  {
    printf("    ERROR: Erase timeout - BSY flag stuck\r\n");
    printf("    SR: 0x%08lX\r\n", FLASH->SR);
  }
  else
  {
    printf("    Erase OK (waited %lu ms)\r\n", 1000 - timeout);
    printf("    Final CR: 0x%08lX, SR: 0x%08lX\r\n", FLASH->CR, FLASH->SR);
  }
  
  /* Clear PER bit */
  CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
  
  /* Step 5: Verify erase */
  printf("[5] Verifying erase...\r\n");
  HAL_Delay(50);  /* Longer delay to ensure Flash is stable */
  uint64_t erasedData = *flashPtr;
  printf("    Data: 0x%08lX%08lX\r\n", 
         (uint32_t)(erasedData >> 32), (uint32_t)(erasedData & 0xFFFFFFFF));
  
  if (erasedData != 0xFFFFFFFFFFFFFFFF)
  {
    printf("    WARNING: Flash not fully erased!\r\n");
    printf("    This explains the SIZE error - can't write to non-erased Flash\r\n");
    printf("    Possible causes:\r\n");
    printf("      1. Flash write protection enabled\r\n");
    printf("      2. Page is in protected region\r\n");
    printf("      3. Hardware fault\r\n");
  }
  else
  {
    printf("    Erase verified OK\r\n");
  }
  
  /* Step 6: Clear CR register */
  printf("[6] Clearing CR register...\r\n");
  printf("    Before: CR = 0x%08lX\r\n", FLASH->CR);
  FLASH->CR = 0;
  printf("    After: CR = 0x%08lX\r\n", FLASH->CR);
  
  /* Step 7: Write test data */
  printf("[7] Writing test data 0x123456789ABCDEF0...\r\n");
  uint64_t testData = 0x123456789ABCDEF0ULL;
  
  printf("    Before write - CR: 0x%08lX, SR: 0x%08lX\r\n", FLASH->CR, FLASH->SR);
  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, TEST_ADDRESS, testData);
  printf("    After write - CR: 0x%08lX, SR: 0x%08lX\r\n", FLASH->CR, FLASH->SR);
  
  if (status != HAL_OK)
  {
    uint32_t flashError = HAL_FLASH_GetError();
    printf("    ERROR: Write failed (status=%d, error=0x%08lX)\r\n", 
           status, flashError);
    HAL_FLASH_Lock();
    while(1);
  }
  printf("    Write OK\r\n");
  
  /* Step 8: Verify write */
  printf("[8] Verifying write...\r\n");
  uint64_t readData = *flashPtr;
  printf("    Expected: 0x123456789ABCDEF0\r\n");
  printf("    Read:     0x%08lX%08lX\r\n", 
         (uint32_t)(readData >> 32), (uint32_t)(readData & 0xFFFFFFFF));
  
  if (readData == testData)
  {
    printf("    PASS: Data verified!\r\n");
  }
  else
  {
    printf("    FAIL: Data mismatch!\r\n");
  }
  
  /* Step 9: Lock Flash */
  printf("[9] Locking Flash...\r\n");
  HAL_FLASH_Lock();
  printf("    CR: 0x%08lX\r\n", FLASH->CR);
  
  printf("\r\n");
  printf("========================================\r\n");
  printf(" Test Complete - System Halted\r\n");
  printf(" Press RESET to run main program\r\n");
  printf("========================================\r\n");
  
  /* Halt here - don't proceed to main program */
  while(1)
  {
    HAL_Delay(1000);
  }
}
