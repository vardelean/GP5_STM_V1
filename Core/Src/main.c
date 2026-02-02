/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"
#include "i2c.h"

/* Private includes ----------------------------------------------------------*/
#include "display.h"
#include "ssd1306.h"
#include "usbh_midi.h"
#include "midi_manager.h"
#include "gp5_midi.h"
#include "preset_buttons.h"
#include <stdio.h>
#include <string.h>
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern USBH_HandleTypeDef hUsbHostFS;

void USB_MIDI_ConnectionCallback(uint8_t connected);
int _write(int file, char *ptr, int len);
uint8_t midi_device_connected = 0;
uint32_t midi_connection_time = 0;  /* Time when MIDI device connected */
uint8_t preset_request_pending = 0; /* Flag to request preset after delay */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);
void USB_MIDI_ConnectionCallback(uint8_t connected);
int _write(int file, char *ptr, int len);

/* Private user code ---------------------------------------------------------*/
/**
 * @brief Retarget printf to UART2
 */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}

/**
 * @brief USB MIDI connection callback
 */
void USB_MIDI_ConnectionCallback(uint8_t connected)
{
  midi_device_connected = connected;

  if (connected)
  {
    uint16_t vid, pid;
    MIDI_Manager_GetDeviceInfo(&vid, &pid);
    printf("MIDI Device Connected - Ready for operation\r\n");

    /* Schedule preset request after delay (handled in main loop) */
    midi_connection_time = HAL_GetTick();
    preset_request_pending = 1;
    printf("[GP-5] Will request preset number after 1 second...\r\n");
  }
  else
  {
    printf("MIDI Device Disconnected\r\n");
    preset_request_pending = 0;
  }
}

/**
 * @brief MIDI receive handler callback (wrapper for GP-5 MIDI processor)
 */
void MIDI_ReceiveHandler(uint8_t *data, uint16_t length)
{
  GP5_MIDI_ProcessReceivedData(data, length);
}

/**
 * @brief Initialize SysEx messages for buttons (wrapper for GP-5 MIDI init)
 */
void InitializeSysExMessages(void)
{
  GP5_MIDI_Init();
}
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  MX_USART2_UART_Init();
  MX_I2C2_Init();
  
  
  // Initialize display
  SSD1306_Init(&hi2c2);
  Display_Init();


  /* Enable USB Power */
  HAL_GPIO_WritePin(USB_PWR_GPIO_Port, USB_PWR_Pin, GPIO_PIN_SET);
  HAL_Delay(3000); // Wait for GP-5 to power up
  printf("USB Power Enabled\r\n");
 

  // Initialize USB Host
  MX_USB_Host_Init();
  printf("\r\n=== STM32 USB MIDI Controller ===\r\n");
  printf("Initializing...\r\n");



  /* Initialize modules */
  MIDI_Manager_Init();
  PresetButtons_Init();

  /* Set USB Host handle for MIDI manager */
  MIDI_Manager_SetUSBHost(&hUsbHostFS);

  /* Register callbacks */
  MIDI_Manager_RegisterReceiveCallback(MIDI_ReceiveHandler);

  /* Initialize SysEx messages */
  InitializeSysExMessages();

  printf("System Ready - Waiting for USB MIDI device...\r\n");
  /* Infinite loop */
  while (1)
  {
    MX_USB_HOST_Process();
    /* Check if system reset was requested by USB error handler */
    extern volatile uint8_t system_reset_requested;
    if (system_reset_requested)
    {
      //printf("\r\n[MAIN] System reset requested due to USB errors\r\n");
      //printf("[MAIN] This enables hot-plug connection recovery\r\n");
      //printf("[MAIN] Resetting in 1 second...\r\n");
      //HAL_Delay(1000);
      //printf("[MAIN] RESET NOW\r\n");
      //HAL_Delay(100);
      NVIC_SystemReset();
    }

    /* Check if preset request is pending (after connection delay) */
    if (preset_request_pending && midi_device_connected)
    {
      uint32_t elapsed = HAL_GetTick() - midi_connection_time;
      if (elapsed >= 1000) /* 1 second has passed */
      {
        printf("[GP-5] Requesting initial preset number...\r\n");
        GP5_MIDI_RequestInitialPreset();
        preset_request_pending = 0; /* Clear flag */
        
        /* Request startup preset recall after initial GP-5 connection */
        printf("[Main] Requesting startup preset recall...\r\n");
        PresetButtons_RequestStartupPresetRecall();
      }
    }

    /* Process MIDI manager */
    MIDI_Manager_Process();

    /* Process button debouncing */
    PresetButtons_Process();
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

/* NOTE: CRS synchronization disabled for USB HOST mode
 * CRS_SYNC_SOURCE_USB only works in USB DEVICE mode (syncs to incoming SOF from host).
 * In USB HOST mode, we generate the SOF, so there's nothing to sync to.
 * HSI48 free-running accuracy should be sufficient for USB HOST.
 * If issues persist, consider using an external crystal (HSE).
 */
#if 0 /* Disable CRS for USB HOST */
   /** Enable the SYSCFG APB clock
  */
  __HAL_RCC_CRS_CLK_ENABLE();

  /** Configures CRS
  */
  pInit.Prescaler = RCC_CRS_SYNC_DIV1;
  pInit.Source = RCC_CRS_SYNC_SOURCE_USB;
  pInit.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  pInit.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  pInit.ErrorLimitValue = 34;
  pInit.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&pInit);
#endif
}
/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
