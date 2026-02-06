/**
  ******************************************************************************
  * @file    midi_manager.c
  * @author  Custom Implementation
  * @brief   MIDI message management and USB HOST interface
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "midi_manager.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MIDI_RX_BUFFER_SIZE   256

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static USBH_HandleTypeDef *usb_host_handle = NULL;
static MIDI_ReceiveCallback_t receive_callback = NULL;
static uint8_t rx_buffer[MIDI_RX_BUFFER_SIZE];
static uint8_t device_connected = 0;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize MIDI manager
  * @retval None
  */
void MIDI_Manager_Init(void)
{
  usb_host_handle = NULL;
  receive_callback = NULL;
  device_connected = 0;
  memset(rx_buffer, 0, sizeof(rx_buffer));
}

/**
  * @brief  Set USB HOST handle
  * @param  phost: USB HOST handle pointer
  * @retval None
  */
void MIDI_Manager_SetUSBHost(USBH_HandleTypeDef *phost)
{
  usb_host_handle = phost;
}

/**
  * @brief  Process MIDI manager (call periodically from main loop)
  * @retval None
  */
void MIDI_Manager_Process(void)
{
  if (usb_host_handle == NULL)
  {
    return;
  }
  
  /* Check if MIDI device is connected */
  if (usb_host_handle->gState == HOST_CLASS)
  {
    if (!device_connected)
    {
      printf("[MIDI_Manager] MIDI device now CONNECTED\r\n");
    }
    device_connected = 1;
    
    /* Check for received data */
    if (USBH_MIDI_ReceiveHasData(usb_host_handle))
    {
      USBH_StatusTypeDef status = USBH_MIDI_Receive(usb_host_handle, rx_buffer, MIDI_RX_BUFFER_SIZE);
      
      if (status == USBH_OK)
      {
        uint16_t length = USBH_MIDI_GetLastReceivedDataSize(usb_host_handle);
        
        if (length > 0 && receive_callback != NULL)
        {
          receive_callback(rx_buffer, length);
        }
      }
    }
  }
  else
  {
    if (device_connected)
    {
      printf("[MIDI_Manager] MIDI device DISCONNECTED (gState changed from HOST_CLASS)\r\n");
    }
    device_connected = 0;
  }
}

/**
  * @brief  Send SysEx message
  * @param  data: SysEx data (including F0 start and F7 end)
  * @param  length: Data length
  * @retval None
  */
void MIDI_Manager_SendSysEx(uint8_t *data, uint16_t length)
{
  if (usb_host_handle == NULL || !device_connected)
  {
    return;
  }
  
  USBH_MIDI_SendSysEx(usb_host_handle, data, length);
}

/**
  * @brief  Send Control Change message
  * @param  channel: MIDI channel (0-15)
  * @param  controller: Controller number (0-127)
  * @param  value: Controller value (0-127)
  * @retval None
  */
void MIDI_Manager_SendCC(uint8_t channel, uint8_t controller, uint8_t value)
{
  if (usb_host_handle == NULL || !device_connected)
  {
    printf("[MIDI_Manager] ERROR: Cannot send CC - device not connected\r\n");
    return;
  }
  
  printf("[MIDI_Manager] Sending CC: Ch%d CC#%d = %d\r\n", channel, controller, value);
  USBH_MIDI_SendControlChange(usb_host_handle, channel, controller, value);
}

/**
  * @brief  Check if MIDI device is connected
  * @retval 1 if connected, 0 otherwise
  */
uint8_t MIDI_Manager_IsDeviceConnected(void)
{
  return device_connected;
}

/**
  * @brief  Get MIDI device VID and PID
  * @param  vid: Pointer to store VID
  * @param  pid: Pointer to store PID
  * @retval None
  */
void MIDI_Manager_GetDeviceInfo(uint16_t *vid, uint16_t *pid)
{
  MIDI_DeviceInfoTypeDef info;
  
  if (usb_host_handle != NULL)
  {
    if (USBH_MIDI_GetDeviceInfo(usb_host_handle, &info) == USBH_OK)
    {
      if (vid != NULL)
      {
        *vid = info.VID;
      }
      if (pid != NULL)
      {
        *pid = info.PID;
      }
    }
  }
}

/**
  * @brief  Register callback for received MIDI messages
  * @param  callback: Callback function pointer
  * @retval None
  */
void MIDI_Manager_RegisterReceiveCallback(MIDI_ReceiveCallback_t callback)
{
  receive_callback = callback;
}
