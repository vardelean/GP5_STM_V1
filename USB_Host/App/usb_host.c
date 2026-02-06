/**
  ******************************************************************************
  * @file            : usb_host.c
  * @version         : v3.0_Cube
  * @brief           : This file implements the USB Host
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

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_midi.h"
#include <stdio.h>
/* Private variables ---------------------------------------------------------*/
static uint32_t disconnect_timestamp = 0;
static uint8_t ignore_early_disconnect = 0;
/* Private function prototypes -----------------------------------------------*/
/* USB Host core handle declaration */
USBH_HandleTypeDef hUsbHostFS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/*
 * -- Insert your variables declaration here --
 */
/*
 * user callback declaration
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/*
 * -- Insert your external function declaration here --
 */
/**
  * Init USB host library, add supported class and start the library
  * @retval None
  */
void MX_USB_Host_Init(void)
{
  /* Init host Library, add supported class and start the library. */
  printf("[MX_USB_Host_Init] Calling USBH_Init...\r\n");
  if (USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS) != USBH_OK)
  {
    printf("[MX_USB_Host_Init] ERROR: USBH_Init failed!\r\n");
    Error_Handler();
  }
  printf("[MX_USB_Host_Init] USBH_Init successful, pUser callback: 0x%08lX\r\n", 
         (unsigned long)hUsbHostFS.pUser);
  
  printf("[MX_USB_Host_Init] Registering MIDI class...\r\n");
  if (USBH_RegisterClass(&hUsbHostFS, USBH_MIDI_CLASS) != USBH_OK)
  {
    printf("[MX_USB_Host_Init] ERROR: USBH_RegisterClass failed!\r\n");
    Error_Handler();
  }
  printf("[MX_USB_Host_Init] MIDI class registered\r\n");
  
  printf("[MX_USB_Host_Init] Starting USB Host...\r\n");
  if (USBH_Start(&hUsbHostFS) != USBH_OK)
  {
    printf("[MX_USB_Host_Init] ERROR: USBH_Start failed!\r\n");
    Error_Handler();
  }
  
  printf("[MX_USB_Host_Init] USB HOST initialized and started\r\n");
}

/*
 * Background task
 */
void MX_USB_HOST_Process(void)
{
  /* USB Host Background task */
  USBH_Process(&hUsbHostFS);
}
/*
 * user callback definition
 */
static void USBH_UserProcess  (USBH_HandleTypeDef *phost, uint8_t id)
{
  extern void USB_MIDI_ConnectionCallback(uint8_t event);
  
  printf("[USBH_UserProcess] Event ID: %d, gState: %d\r\n", id, phost->gState);
  
  switch(id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
  printf("USB: Configuration Selected\r\n");
  break;

  case HOST_USER_DISCONNECTION:
  /* Filter spurious disconnects during device initialization (GP-5 boot-up)
   * Ignore disconnect events within 1 second of connection
   */
  if (ignore_early_disconnect && (HAL_GetTick() - disconnect_timestamp < 1000))
  {
    printf("USB: Ignoring early disconnect (device initializing)\r\n");
    break;
  }
  
  Appli_state = APPLICATION_DISCONNECT;
  printf("USB MIDI Device Disconnected\r\n");
  USB_MIDI_ConnectionCallback(0);
  ignore_early_disconnect = 0;
  break;

  case HOST_USER_CLASS_ACTIVE:
  Appli_state = APPLICATION_READY;
  {
    MIDI_DeviceInfoTypeDef info;
    if (USBH_MIDI_GetDeviceInfo(phost, &info) == USBH_OK)
  disconnect_timestamp = HAL_GetTick();
  ignore_early_disconnect = 1;  /* Ignore disconnects for next 1 second */
    {
      printf("USB MIDI Device Connected - VID: 0x%04X, PID: 0x%04X\r\n", info.VID, info.PID);
      USB_MIDI_ConnectionCallback(1);
    }
  }
  break;

  case HOST_USER_CONNECTION:
  Appli_state = APPLICATION_START;
  printf("USB Device Detected - Starting Enumeration\r\n");
  break;
  
  case HOST_USER_CLASS_SELECTED:
  printf("USB: MIDI Class Selected\r\n");
  break;
  
  case HOST_USER_UNRECOVERED_ERROR:
  printf("USB ERROR: Unrecovered Error!\r\n");
  break;

  default:
  printf("USB: Event ID %d\r\n", id);
  break;
  }
}

/**
  * @}
  */

/**
  * @}
  */

