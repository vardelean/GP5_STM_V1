/**
  ******************************************************************************
  * @file    midi_manager.h
  * @author  Custom Implementation
  * @brief   Header for midi_manager.c file
  ******************************************************************************
  */

#ifndef __MIDI_MANAGER_H
#define __MIDI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include "usbh_midi.h"
#include "usb_host.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  MIDI_MSG_SYSEX = 0,
  MIDI_MSG_CC,
  MIDI_MSG_NOTE_ON,
  MIDI_MSG_NOTE_OFF
} MIDI_MessageType_t;

typedef struct
{
  uint8_t data[256];
  uint16_t length;
} MIDI_SysExMessage_t;

typedef struct
{
  uint8_t channel;
  uint8_t controller;
  uint8_t value;
} MIDI_CCMessage_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void MIDI_Manager_Init(void);
void MIDI_Manager_Process(void);
void MIDI_Manager_SendSysEx(uint8_t *data, uint16_t length);
void MIDI_Manager_SendCC(uint8_t channel, uint8_t controller, uint8_t value);
void MIDI_Manager_SetUSBHost(USBH_HandleTypeDef *phost);
uint8_t MIDI_Manager_IsDeviceConnected(void);
void MIDI_Manager_GetDeviceInfo(uint16_t *vid, uint16_t *pid);

/* Callback for received MIDI messages */
typedef void (*MIDI_ReceiveCallback_t)(uint8_t *data, uint16_t length);
void MIDI_Manager_RegisterReceiveCallback(MIDI_ReceiveCallback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __MIDI_MANAGER_H */
