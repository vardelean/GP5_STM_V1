/**
  ******************************************************************************
  * @file    usbh_midi.h
  * @author  Custom Implementation
  * @brief   Header file for usbh_midi.c
  ******************************************************************************
  */

#ifndef __USBH_MIDI_H
#define __USBH_MIDI_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"

/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_MIDI_CLASS
  * @{
  */

/** @defgroup USBH_MIDI_CORE
  * @brief This file is the Header file for usbh_midi.c
  * @{
  */


/*MIDI Class Codes*/
#define USB_AUDIO_CLASS                                 0x01U
#define AUDIO_SUBCLASS_MIDISTREAMING                    0x03U

/* MIDI Packet Structure */
#define MIDI_PACKET_SIZE                                64U

/* MIDI USB Event Packet */
typedef struct
{
  uint8_t CIN_CN;       /* Cable Number and Code Index Number */
  uint8_t MIDI_0;       /* MIDI_0 */
  uint8_t MIDI_1;       /* MIDI_1 */
  uint8_t MIDI_2;       /* MIDI_2 */
} MIDI_EventPacketTypeDef;

/* States for MIDI State Machine */
typedef enum
{
  MIDI_INIT = 0,
  MIDI_IDLE,
  MIDI_SEND,
  MIDI_RECEIVE,
  MIDI_POLL,
  MIDI_ERROR,
}
MIDI_StateTypeDef;

/* Structure for MIDI process */
typedef struct
{
  uint8_t              OutPipe;
  uint8_t              InPipe;
  MIDI_StateTypeDef    state;
  uint8_t              OutEp;
  uint8_t              InEp;
  uint16_t             OutEpSize;
  uint16_t             InEpSize;
  uint8_t              buff[MIDI_PACKET_SIZE];
  uint16_t             length;
  uint16_t             poll;
  uint32_t             timer;
  uint8_t              DataReady;
  USBH_StatusTypeDef   (*Init)(USBH_HandleTypeDef *phost);
}
USBH_MIDI_HandleTypeDef;

/* MIDI Device Info */
typedef struct
{
  uint16_t VID;
  uint16_t PID;
  uint8_t  InterfaceNumber;
  uint8_t  Connected;
}
MIDI_DeviceInfoTypeDef;


/**
  * @}
  */

/** @defgroup USBH_MIDI_CORE_Exported_Defines
  * @{
  */

/**
  * @}
  */

/** @defgroup USBH_MIDI_CORE_Exported_Macros
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_MIDI_CORE_Exported_Variables
  * @{
  */
extern USBH_ClassTypeDef  MIDI_Class;
#define USBH_MIDI_CLASS    &MIDI_Class

extern MIDI_DeviceInfoTypeDef MIDI_DeviceInfo;
/**
  * @}
  */

/** @defgroup USBH_MIDI_CORE_Exported_FunctionsPrototype
  * @{
  */

USBH_StatusTypeDef USBH_MIDI_Transmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint16_t length);
USBH_StatusTypeDef USBH_MIDI_Receive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint16_t length);
uint16_t USBH_MIDI_GetLastReceivedDataSize(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_MIDI_GetDeviceInfo(USBH_HandleTypeDef *phost, MIDI_DeviceInfoTypeDef *info);

/* Helper functions for MIDI messages */
void USBH_MIDI_SendControlChange(USBH_HandleTypeDef *phost, uint8_t channel, uint8_t controller, uint8_t value);
void USBH_MIDI_SendSysEx(USBH_HandleTypeDef *phost, uint8_t *data, uint16_t length);
uint8_t USBH_MIDI_ReceiveHasData(USBH_HandleTypeDef *phost);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBH_MIDI_H */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
