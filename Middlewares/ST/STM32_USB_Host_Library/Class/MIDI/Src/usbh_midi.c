/**
  ******************************************************************************
  * @file    usbh_midi.c
  * @author  Custom Implementation
  * @brief   USB Host MIDI class driver
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbh_midi.h"

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
  * @brief    This file includes MIDI Layer Handlers for USB Host MIDI class.
  * @{
  */

/** @defgroup USBH_MIDI_CORE_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_MIDI_CORE_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_MIDI_CORE_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_MIDI_CORE_Private_Variables
  * @{
  */

MIDI_DeviceInfoTypeDef MIDI_DeviceInfo;

/**
  * @}
  */


/** @defgroup USBH_MIDI_CORE_Private_FunctionPrototypes
  * @{
  */

static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost);

USBH_ClassTypeDef  MIDI_Class =
{
  "MIDI",
  USB_AUDIO_CLASS,  /* 0x01 - Standard Audio class (GP-5), also accepts 0xFF vendor-specific (Roland UNO) */
  USBH_MIDI_InterfaceInit,
  USBH_MIDI_InterfaceDeInit,
  USBH_MIDI_ClassRequest,
  USBH_MIDI_Process,
  USBH_MIDI_SOFProcess,
  NULL,
};

/**
  * @}
  */


/** @defgroup USBH_MIDI_CORE_Private_Functions
  * @{
  */

/**
  * @brief  USBH_MIDI_InterfaceInit
  *         The function init the MIDI class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MIDI_InterfaceInit(USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_FAIL;
  USBH_MIDI_HandleTypeDef *MIDI_Handle;
  uint8_t interface;

  USBH_DbgLog("MIDI_InterfaceInit: Starting MIDI class initialization");
  
  /* Log all available interfaces for debugging */
  USBH_DbgLog("Device has %d interfaces:", phost->device.CfgDesc.bNumInterfaces);
  for (uint8_t i = 0; i < phost->device.CfgDesc.bNumInterfaces && i < USBH_MAX_NUM_INTERFACES; i++)
  {
    USBH_DbgLog("  Interface[%d]: Class=0x%02X, SubClass=0x%02X, Protocol=0x%02X",
                i,
                phost->device.CfgDesc.Itf_Desc[i].bInterfaceClass,
                phost->device.CfgDesc.Itf_Desc[i].bInterfaceSubClass,
                phost->device.CfgDesc.Itf_Desc[i].bInterfaceProtocol);
  }
  
  /* Find MIDI streaming interface 
   * Try vendor-specific class (0xFF) first for Roland devices, 
   * then fall back to standard Audio class (0x01) */
  interface = USBH_FindInterface(phost, 0xFF, AUDIO_SUBCLASS_MIDISTREAMING, 0xFF);
  
  if (interface == 0xFFU)
  {
    /* Try standard Audio class as fallback */
    interface = USBH_FindInterface(phost, USB_AUDIO_CLASS, AUDIO_SUBCLASS_MIDISTREAMING, 0xFF);
  }
  
  USBH_DbgLog("MIDI_InterfaceInit: Interface search result = 0x%02X", interface);

  if (interface == 0xFFU) /* No Valid Interface */
  {
    USBH_DbgLog("ERROR: Cannot Find the interface for MIDI Class.");
    USBH_DbgLog("Device Class=0x%02X, SubClass=0x%02X", 
                phost->device.DevDesc.bDeviceClass,
                phost->device.DevDesc.bDeviceSubClass);
    return status;
  }
  else
  {
    USBH_DbgLog("MIDI_InterfaceInit: Found MIDI interface %d, selecting...", interface);
    USBH_SelectInterface(phost, interface);
    phost->pActiveClass->pData = (USBH_MIDI_HandleTypeDef *)USBH_malloc(sizeof(USBH_MIDI_HandleTypeDef));
    MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;

    if (MIDI_Handle == NULL)
    {
      USBH_DbgLog("Cannot allocate memory for MIDI Handle");
      return USBH_FAIL;
    }

    /* Initialize MIDI handler */
    USBH_memset(MIDI_Handle, 0, sizeof(USBH_MIDI_HandleTypeDef));

    /* Get MIDI Endpoints */
    if (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress & 0x80U)
    {
      MIDI_Handle->InEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
      MIDI_Handle->InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
    }
    else
    {
      MIDI_Handle->OutEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
      MIDI_Handle->OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
    }

    if (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress & 0x80U)
    {
      MIDI_Handle->InEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress;
      MIDI_Handle->InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
    }
    else
    {
      MIDI_Handle->OutEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress;
      MIDI_Handle->OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
    }

    /* Open pipes */
    MIDI_Handle->OutPipe = USBH_AllocPipe(phost, MIDI_Handle->OutEp);
    MIDI_Handle->InPipe = USBH_AllocPipe(phost, MIDI_Handle->InEp);

    /* Open the new channels */
    USBH_OpenPipe(phost, MIDI_Handle->OutPipe, MIDI_Handle->OutEp, phost->device.address,
                  phost->device.speed, USB_EP_TYPE_BULK, MIDI_Handle->OutEpSize);

    USBH_OpenPipe(phost, MIDI_Handle->InPipe, MIDI_Handle->InEp, phost->device.address,
                  phost->device.speed, USB_EP_TYPE_BULK, MIDI_Handle->InEpSize);

    USBH_LL_SetToggle(phost, MIDI_Handle->InPipe, 0U);
    USBH_LL_SetToggle(phost, MIDI_Handle->OutPipe, 0U);

    MIDI_Handle->state = MIDI_IDLE;
    MIDI_Handle->DataReady = 0U;

    /* Store device info */
    MIDI_DeviceInfo.VID = phost->device.DevDesc.idVendor;
    MIDI_DeviceInfo.PID = phost->device.DevDesc.idProduct;
    MIDI_DeviceInfo.InterfaceNumber = interface;
    MIDI_DeviceInfo.Connected = 1U;

    USBH_DbgLog("MIDI_InterfaceInit: SUCCESS - VID=0x%04X PID=0x%04X",
                MIDI_DeviceInfo.VID, MIDI_DeviceInfo.PID);
    USBH_DbgLog("MIDI_InterfaceInit: InEP=0x%02X OutEP=0x%02X",
                MIDI_Handle->InEp, MIDI_Handle->OutEp);
    
    status = USBH_OK;
  }

  return status;
}

/**
  * @brief  USBH_MIDI_InterfaceDeInit
  *         The function DeInit the Pipes used for the MIDI class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit(USBH_HandleTypeDef *phost)
{
  USBH_MIDI_HandleTypeDef *MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;

  if (MIDI_Handle != NULL)
  {
    if (MIDI_Handle->OutPipe != 0U)
    {
      USBH_ClosePipe(phost, MIDI_Handle->OutPipe);
      USBH_FreePipe(phost, MIDI_Handle->OutPipe);
      MIDI_Handle->OutPipe = 0U;     /* Reset the pipe as Free */
    }

    if (MIDI_Handle->InPipe != 0U)
    {
      USBH_ClosePipe(phost, MIDI_Handle->InPipe);
      USBH_FreePipe(phost, MIDI_Handle->InPipe);
      MIDI_Handle->InPipe = 0U;     /* Reset the pipe as Free */
    }

    if (phost->pActiveClass->pData != NULL)
    {
      USBH_free(phost->pActiveClass->pData);
      phost->pActiveClass->pData = NULL;
    }
  }

  /* Clear device info */
  MIDI_DeviceInfo.Connected = 0U;

  return USBH_OK;
}

/**
  * @brief  USBH_MIDI_ClassRequest
  *         The function is responsible for handling Standard requests
  *         for MIDI class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)
{
  /* MIDI class doesn't require class-specific requests */
  phost->pUser(phost, HOST_USER_CLASS_ACTIVE);
  return USBH_OK;
}

/**
  * @brief  USBH_MIDI_Process
  *         The function is for managing state machine for MIDI data transfers
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_OK;
  USBH_MIDI_HandleTypeDef *MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;
  USBH_URBStateTypeDef URB_Status = USBH_URB_IDLE;

  if (MIDI_Handle == NULL)
  {
    return USBH_FAIL;
  }

  switch (MIDI_Handle->state)
  {
    case MIDI_INIT:
      MIDI_Handle->state = MIDI_IDLE;
      break;

    case MIDI_IDLE:
      /* Start receiving data */
      USBH_BulkReceiveData(phost, MIDI_Handle->buff, MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
      MIDI_Handle->state = MIDI_POLL;
      break;

    case MIDI_POLL:
      URB_Status = USBH_LL_GetURBState(phost, MIDI_Handle->InPipe);

      if (URB_Status == USBH_URB_DONE)
      {
        /* Data received */
        MIDI_Handle->length = USBH_LL_GetLastXferSize(phost, MIDI_Handle->InPipe);

        if (MIDI_Handle->length > 0U)
        {
          MIDI_Handle->DataReady = 1U;
        }

        /* Restart receiving */
        USBH_BulkReceiveData(phost, MIDI_Handle->buff, MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
      }
      else if (URB_Status == USBH_URB_STALL)
      {
        /* Issue Clear Feature on interrupt IN endpoint */
        if (USBH_ClrFeature(phost, MIDI_Handle->InEp) == USBH_OK)
        {
          /* Change state to issue next IN token */
          MIDI_Handle->state = MIDI_IDLE;
        }
      }
      break;

    default:
      break;
  }

  return status;
}

/**
  * @brief  USBH_MIDI_SOFProcess
  *         The function is for managing SOF callback
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(phost);

  return USBH_OK;
}

/**
  * @brief  USBH_MIDI_Transmit
  *         Send MIDI data
  * @param  phost: Host handle
  * @param  pbuff: Buffer pointer
  * @param  length: Length of data
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_MIDI_Transmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint16_t length)
{
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_MIDI_HandleTypeDef *MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;

  if (MIDI_Handle != NULL)
  {
    if (MIDI_Handle->state == MIDI_IDLE || MIDI_Handle->state == MIDI_POLL)
    {
      status = USBH_BulkSendData(phost, pbuff, length, MIDI_Handle->OutPipe, 1U);
    }
  }

  return status;
}

/**
  * @brief  USBH_MIDI_Receive
  *         Receive MIDI data (non-blocking)
  * @param  phost: Host handle
  * @param  pbuff: Buffer pointer
  * @param  length: Maximum length
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_MIDI_Receive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint16_t length)
{
  USBH_StatusTypeDef status = USBH_FAIL;
  USBH_MIDI_HandleTypeDef *MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;

  if ((MIDI_Handle != NULL) && (MIDI_Handle->DataReady == 1U))
  {
    uint16_t copy_length = (MIDI_Handle->length < length) ? MIDI_Handle->length : length;
    USBH_memcpy(pbuff, MIDI_Handle->buff, copy_length);
    MIDI_Handle->DataReady = 0U;
    status = USBH_OK;
  }

  return status;
}

/**
  * @brief  USBH_MIDI_GetLastReceivedDataSize
  *         Return the last received data size
  * @param  phost: Host handle
  * @retval Data Size
  */
uint16_t USBH_MIDI_GetLastReceivedDataSize(USBH_HandleTypeDef *phost)
{
  USBH_MIDI_HandleTypeDef *MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;
  uint16_t size = 0U;

  if (MIDI_Handle != NULL)
  {
    size = MIDI_Handle->length;
  }

  return size;
}

/**
  * @brief  USBH_MIDI_GetDeviceInfo
  *         Get MIDI device information (VID, PID)
  * @param  phost: Host handle
  * @param  info: Pointer to device info structure
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_MIDI_GetDeviceInfo(USBH_HandleTypeDef *phost, MIDI_DeviceInfoTypeDef *info)
{
  if (info != NULL)
  {
    info->VID = MIDI_DeviceInfo.VID;
    info->PID = MIDI_DeviceInfo.PID;
    info->InterfaceNumber = MIDI_DeviceInfo.InterfaceNumber;
    info->Connected = MIDI_DeviceInfo.Connected;
    return USBH_OK;
  }

  return USBH_FAIL;
}

/**
  * @brief  USBH_MIDI_SendControlChange
  *         Send MIDI Control Change message
  * @param  phost: Host handle
  * @param  channel: MIDI channel (0-15)
  * @param  controller: Controller number (0-127)
  * @param  value: Controller value (0-127)
  * @retval None
  */
void USBH_MIDI_SendControlChange(USBH_HandleTypeDef *phost, uint8_t channel, uint8_t controller, uint8_t value)
{
  uint8_t packet[4];

  /* USB-MIDI packet for Control Change */
  packet[0] = 0x0B;                       /* Cable 0, Code Index 0xB (Control Change) */
  packet[1] = 0xB0 | (channel & 0x0F);    /* Control Change + channel */
  packet[2] = controller & 0x7F;          /* Controller number */
  packet[3] = value & 0x7F;               /* Controller value */

  USBH_MIDI_Transmit(phost, packet, 4);
}

/**
  * @brief  USBH_MIDI_SendSysEx
  *         Send MIDI System Exclusive message
  * @param  phost: Host handle
  * @param  data: SysEx data (including F0 and F7)
  * @param  length: Data length
  * @retval None
  */
void USBH_MIDI_SendSysEx(USBH_HandleTypeDef *phost, uint8_t *data, uint16_t length)
{
  uint8_t packet[64];
  uint16_t packet_idx = 0;
  uint16_t data_idx = 0;
  uint8_t cin;

  while (data_idx < length)
  {
    uint16_t remaining = length - data_idx;

    if (data_idx == 0)
    {
      /* First packet */
      if (remaining == 1)
      {
        cin = 0x05; /* Single-byte system common */
      }
      else if (remaining == 2)
      {
        cin = 0x06; /* Two-byte system common */
      }
      else if (remaining >= 3)
      {
        cin = 0x04; /* SysEx starts */
      }
    }
    else if (remaining == 1)
    {
      cin = 0x05; /* SysEx ends with one byte */
    }
    else if (remaining == 2)
    {
      cin = 0x06; /* SysEx ends with two bytes */
    }
    else if (remaining >= 3)
    {
      if (data[data_idx + 2] == 0xF7)
      {
        cin = 0x07; /* SysEx ends with three bytes */
      }
      else
      {
        cin = 0x04; /* SysEx continues */
      }
    }

    /* Build packet */
    packet[packet_idx++] = cin;
    packet[packet_idx++] = data[data_idx++];
    packet[packet_idx++] = (data_idx < length) ? data[data_idx++] : 0;
    packet[packet_idx++] = (data_idx < length) ? data[data_idx++] : 0;

    /* Send when packet buffer is full or all data processed */
    if (packet_idx >= 60 || data_idx >= length)
    {
      USBH_MIDI_Transmit(phost, packet, packet_idx);
      packet_idx = 0;
    }
  }
}

/**
  * @brief  USBH_MIDI_ReceiveHasData
  *         Check if MIDI data is available
  * @param  phost: Host handle
  * @retval 1 if data available, 0 otherwise
  */
uint8_t USBH_MIDI_ReceiveHasData(USBH_HandleTypeDef *phost)
{
  USBH_MIDI_HandleTypeDef *MIDI_Handle = (USBH_MIDI_HandleTypeDef *) phost->pActiveClass->pData;

  if (MIDI_Handle != NULL)
  {
    return MIDI_Handle->DataReady;
  }

  return 0;
}

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

/**
  * @}
  */
