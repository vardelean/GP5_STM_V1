/**
  ******************************************************************************
  * @file           : Target/usbh_conf.c
  * @version        : v3.0_Cube
  * @brief          : This file implements the board support package for the USB host library
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
#include "usbh_core.h"
#include "usbh_platform.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
HCD_HandleTypeDef hhcd_USB_DRD_FS;
void Error_Handler(void);
static uint32_t usb_host_start_time = 0;  /* Time when USB HOST started */
static uint32_t last_disconnect_time = 0;  /* Time of last disconnect event */
static uint32_t consecutive_disconnect_count = 0;  /* Count rapid disconnects */
static uint8_t usb_restart_disabled = 0;  /* Stop restart loop if set */
volatile uint8_t system_reset_requested = 0;  /* Flag for main loop to trigger reset */
#define USB_RECONNECT_DEBOUNCE_MS 2000  /* Wait 2 seconds before allowing reconnect */
#define MAX_CONSECUTIVE_DISCONNECTS 3  /* Stop restarting after this many */
/* Private function prototypes -----------------------------------------------*/
USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status);
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
                       LL Driver Callbacks (HCD -> USB Host Library)
*******************************************************************************/
/* MSP Init */

void HAL_HCD_MspInit(HCD_HandleTypeDef* hcdHandle)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hcdHandle->Instance==USB_DRD_FS)
  {
  /* Set start time for disconnect filtering */
  usb_host_start_time = HAL_GetTick();
  printf("[HAL_HCD_MspInit] USB host initialization started at %lu ms\r\n", usb_host_start_time);
  /** Initializes the peripherals clocks
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_USB_CLK_ENABLE();

    /* Enable VDDUSB */
    if(__HAL_RCC_PWR_IS_CLK_DISABLED())
    {
      __HAL_RCC_PWR_CLK_ENABLE();
      HAL_PWREx_EnableVddUSB();
      __HAL_RCC_PWR_CLK_DISABLE();
    }
    else
    {
      HAL_PWREx_EnableVddUSB();
    }

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USB_UCPD1_2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
  }
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef* hcdHandle)
{
  if(hcdHandle->Instance==USB_DRD_FS)
  {
    /* Disable Peripheral clock */
    __HAL_RCC_USB_CLK_DISABLE();

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
  }
}

/**
  * @brief  SOF callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
  USBH_LL_IncTimer(hhcd->pData);
}

/**
  * @brief  SOF callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
  uint32_t elapsed = HAL_GetTick() - usb_host_start_time;
  
  /* During first 1000ms, only allow first connection (filter reconnects from GP-5 boot) */
  if (elapsed < 1000)
  {
    static uint8_t first_connect_done = 0;
    if (first_connect_done)
    {
      printf("[HCD] *** FILTERED RECONNECT during boot (%lums) ***\r\n", elapsed);
      return;
    }
    first_connect_done = 1;
  }
  
  printf("[HCD] Device physically connected\r\n");
  USBH_LL_Connect(hhcd->pData);
}

/**
  * @brief  SOF callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
  /* If usb_host_start_time hasn't been set yet, ignore disconnect */
  if (usb_host_start_time == 0)
  {
    printf("[HCD] *** FILTERED DISCONNECT (not initialized yet) ***\r\n");
    return;
  }
  
  uint32_t elapsed = HAL_GetTick() - usb_host_start_time;
  
  /* During first 1000ms, filter all disconnects (GP-5 boots with D+ low) */
  if (elapsed < 1000)
  {
    printf("[HCD] *** FILTERED DISCONNECT during boot (%lums) ***\r\n", elapsed);
    return;
  }
  
  /* Count ALL disconnects after boot period */
  consecutive_disconnect_count++;
  
  printf("[HCD] Device physically disconnected (count: %lu)\r\n", consecutive_disconnect_count);
  
  /* If too many disconnects, request system reset */
  if (consecutive_disconnect_count >= MAX_CONSECUTIVE_DISCONNECTS)
  {
    printf("[HCD] *** TOO MANY DISCONNECTS - Requesting system reset ***\r\n");
    system_reset_requested = 1;  /* Let main loop handle reset */
    usb_restart_disabled = 1;  /* Stop USB restarts */
    return;  /* Don't process this disconnect further */
  }
  
  last_disconnect_time = HAL_GetTick();
  USBH_LL_Disconnect(hhcd->pData);
}

/**
  * @brief  Notify URB state change callback.
  * @param  hhcd: HCD handle
  * @param  chnum: channel number
  * @param  urb_state: state
  * @retval None
  */
void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state)
{
  /* To be used with OS to sync URB state with the global state machine */
#if (USBH_USE_OS == 1)
  USBH_LL_NotifyURBChange(hhcd->pData);
#endif
}
/**
* @brief  Port Port Enabled callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
  printf("[HCD] USB Port enabled\r\n");
  USBH_LL_PortEnabled(hhcd->pData);
}

/**
  * @brief  Port Port Disabled callback.
  * @param  hhcd: HCD handle
  * @retval None
  */
void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
  printf("[HCD] USB Port disabled\r\n");
  USBH_LL_PortDisabled(hhcd->pData);
}

/*******************************************************************************
                       LL Driver Interface (USB Host Library --> HCD)
*******************************************************************************/

/**
  * @brief  Initialize the low level portion of the host driver.
  * @param  phost: Host handle
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
  printf("[USBH_LL_Init] Initializing USB HOST Low Level\r\n");
  /* Init USB_IP */
  if (phost->id == HOST_FS) {
  /* Link the driver to the stack. */
  hhcd_USB_DRD_FS.pData = phost;
  phost->pData = &hhcd_USB_DRD_FS;

  hhcd_USB_DRD_FS.Instance = USB_DRD_FS;
  hhcd_USB_DRD_FS.Init.dev_endpoints = 8;
  hhcd_USB_DRD_FS.Init.Host_channels = 8;
  hhcd_USB_DRD_FS.Init.speed = HCD_SPEED_FULL;
  hhcd_USB_DRD_FS.Init.phy_itface = HCD_PHY_EMBEDDED;
  hhcd_USB_DRD_FS.Init.Sof_enable = DISABLE;
  hhcd_USB_DRD_FS.Init.low_power_enable = DISABLE;
  hhcd_USB_DRD_FS.Init.lpm_enable = DISABLE;
  hhcd_USB_DRD_FS.Init.battery_charging_enable = ENABLE;  /* Enable BCD for GP-5 detection */
  hhcd_USB_DRD_FS.Init.vbus_sensing_enable = DISABLE;
  hhcd_USB_DRD_FS.Init.bulk_doublebuffer_enable = DISABLE;
  hhcd_USB_DRD_FS.Init.iso_singlebuffer_enable = DISABLE;
  printf("[USBH_LL_Init] Calling HAL_HCD_Init (with BCD enabled)...\r\n");
  if (HAL_HCD_Init(&hhcd_USB_DRD_FS) != HAL_OK)
  {
    printf("[USBH_LL_Init] ERROR: HAL_HCD_Init failed!\r\n");
    Error_Handler( );
  }
  printf("[USBH_LL_Init] HAL_HCD_Init successful\r\n");
  
  /* Add delay for USB peripheral to stabilize */
  HAL_Delay(100);
  
  /* Force USB HOST mode - disable device mode detection */
  /* This helps when using direct wire connections without proper USB connector */
  hhcd_USB_DRD_FS.Instance->CNTR |= USB_CNTR_HOST;  /* Force HOST mode */
  
  /* CRITICAL FIX: Manually enable D+ pull-down for HOST mode
   * Some USB devices (like GP-5) check for HOST pull-downs before enabling their pull-up
   * The STM32G0 HAL doesn't always configure this correctly in HOST mode
   */
  hhcd_USB_DRD_FS.Instance->BCDR |= USB_BCDR_DPPD;  /* Enable D+ pull-down (15kΩ to GND) */
  
  /* Check USB registers for diagnostics */
  printf("[USBH_LL_Init] USB State: 0x%08lX\r\n", (unsigned long)hhcd_USB_DRD_FS.Instance->CNTR);
  printf("[USBH_LL_Init] USB BCDR: 0x%08lX\r\n", (unsigned long)hhcd_USB_DRD_FS.Instance->BCDR);

  USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd_USB_DRD_FS));
  }
  printf("[USBH_LL_Init] Complete\r\n");
  return USBH_OK;
}

/**
  * @brief  De-Initialize the low level portion of the host driver.
  * @param  phost: Host handle
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  hal_status = HAL_HCD_DeInit(phost->pData);

  usb_status = USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Start the low level portion of the host driver.
  * @param  phost: Host handle
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  /* Check if restart is disabled due to too many failures */
  if (usb_restart_disabled)
  {
    printf("[USBH_LL_Start] *** RESTART DISABLED - Manual reset required ***\r\n");
    return USBH_FAIL;
  }

  printf("[USBH_LL_Start] Starting HCD...\r\n");
  
  /* Add delay to prevent rapid restart loop */
  HAL_Delay(500);
  
  /* NOTE: Do NOT update usb_host_start_time here - it's set once in MSP init */
  hal_status = HAL_HCD_Start(phost->pData);
  printf("[USBH_LL_Start] HAL_HCD_Start returned: %d\r\n", hal_status);
  
  /* CRITICAL: Enable D+ pull-down AFTER starting HCD
   * HAL_HCD_Start may reset BCDR register, so we must set it here
   */
  hhcd_USB_DRD_FS.Instance->BCDR |= USB_BCDR_DPPD;  /* Enable D+ pull-down (15kΩ) */
  printf("[USBH_LL_Start] BCDR after pull-down enable: 0x%08lX\r\n", 
         (unsigned long)hhcd_USB_DRD_FS.Instance->BCDR);

  usb_status = USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Stop the low level portion of the host driver.
  * @param  phost: Host handle
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  printf("[USBH_LL_Stop] Stopping HCD...\r\n");
  hal_status = HAL_HCD_Stop(phost->pData);

  usb_status = USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Return the USB host speed from the low level driver.
  * @param  phost: Host handle
  * @retval USBH speeds
  */
USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
  USBH_SpeedTypeDef speed = USBH_SPEED_FULL;

  switch (HAL_HCD_GetCurrentSpeed(phost->pData))
  {
  case 0 :
    speed = USBH_SPEED_HIGH;
    break;

  case 1 :
    speed = USBH_SPEED_FULL;
    break;

  case 2 :
    speed = USBH_SPEED_LOW;
    break;

  default:
   speed = USBH_SPEED_FULL;
    break;
  }
  return  speed;
}

/**
  * @brief  Reset the Host port of the low level driver.
  * @param  phost: Host handle
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  hal_status = HAL_HCD_ResetPort(phost->pData);

  usb_status = USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Return the last transferred packet size.
  * @param  phost: Host handle
  * @param  pipe: Pipe index
  * @retval Packet size
  */
uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
}

/**
  * @brief  Open a pipe of the low level driver.
  * @param  phost: Host handle
  * @param  pipe_num: Pipe index
  * @param  epnum: Endpoint number
  * @param  dev_address: Device USB address
  * @param  speed: Device Speed
  * @param  ep_type: Endpoint type
  * @param  mps: Endpoint max packet size
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe_num, uint8_t epnum,
                                    uint8_t dev_address, uint8_t speed, uint8_t ep_type, uint16_t mps)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  hal_status = HAL_HCD_HC_Init(phost->pData, pipe_num, epnum,
                               dev_address, speed, ep_type, mps);

  usb_status = USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Close a pipe of the low level driver.
  * @param  phost: Host handle
  * @param  pipe: Pipe index
  * @retval USBH status
  */
USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  hal_status = HAL_HCD_HC_Halt(phost->pData, pipe);

  usb_status = USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Submit a new URB to the low level driver.
  * @param  phost: Host handle
  * @param  pipe: Pipe index
  *         This parameter can be a value from 1 to 15
  * @param  direction : Channel number
  *          This parameter can be one of the these values:
  *           0 : Output
  *           1 : Input
  * @param  ep_type : Endpoint Type
  *          This parameter can be one of the these values:
  *            @arg EP_TYPE_CTRL: Control type
  *            @arg EP_TYPE_ISOC: Isochrounous type
  *            @arg EP_TYPE_BULK: Bulk type
  *            @arg EP_TYPE_INTR: Interrupt type
  * @param  token : Endpoint Type
  *          This parameter can be one of the these values:
  *            @arg 0: PID_SETUP
  *            @arg 1: PID_DATA
  * @param  pbuff : pointer to URB data
  * @param  length : Length of URB data
  * @param  do_ping : activate do ping protocol (for high speed only)
  *          This parameter can be one of the these values:
  *           0 : do ping inactive
  *           1 : do ping active
  * @retval Status
  */
USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t direction,
                                     uint8_t ep_type, uint8_t token, uint8_t *pbuff, uint16_t length,
                                     uint8_t do_ping)
{
  HAL_StatusTypeDef hal_status = HAL_OK;
  USBH_StatusTypeDef usb_status = USBH_OK;

  hal_status = HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction ,
                                        ep_type, token, pbuff, length,
                                        do_ping);
  usb_status =  USBH_Get_USB_Status(hal_status);

  return usb_status;
}

/**
  * @brief  Get a URB state from the low level driver.
  * @param  phost: Host handle
  * @param  pipe: Pipe index
  *         This parameter can be a value from 1 to 15
  * @retval URB state
  *          This parameter can be one of the these values:
  *            @arg URB_IDLE
  *            @arg URB_DONE
  *            @arg URB_NOTREADY
  *            @arg URB_NYET
  *            @arg URB_ERROR
  *            @arg URB_STALL
  */
USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState (phost->pData, pipe);
}

/**
  * @brief  Drive VBUS.
  * @param  phost: Host handle
  * @param  state : VBUS state
  *          This parameter can be one of the these values:
  *           0 : VBUS Inactive
  *           1 : VBUS Active
  * @retval Status
  */
USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
  if (phost->id == HOST_FS) {
    MX_DriverVbusFS(state);
  }
  HAL_Delay(200);
  return USBH_OK;
}

/**
  * @brief  Set toggle for a pipe.
  * @param  phost: Host handle
  * @param  pipe: Pipe index
  * @param  toggle: toggle (0/1)
  * @retval Status
  */
USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
  HCD_HandleTypeDef *pHandle;
  pHandle = phost->pData;

  if(pHandle->hc[pipe].ch_dir)
  {
    pHandle->hc[pipe].toggle_in = toggle;
  }
  else
  {
    pHandle->hc[pipe].toggle_out = toggle;
  }

  return USBH_OK;
}

/**
  * @brief  Return the current toggle of a pipe.
  * @param  phost: Host handle
  * @param  pipe: Pipe index
  * @retval toggle (0/1)
  */
uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  uint8_t toggle = 0;
  HCD_HandleTypeDef *pHandle;
  pHandle = phost->pData;

  if(pHandle->hc[pipe].ch_dir)
  {
    toggle = pHandle->hc[pipe].toggle_in;
  }
  else
  {
    toggle = pHandle->hc[pipe].toggle_out;
  }
  return toggle;
}

/**
  * @brief  Delay routine for the USB Host Library
  * @param  Delay: Delay in ms
  * @retval None
  */
void USBH_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

/**
  * @brief  Returns the USB status depending on the HAL status:
  * @param  hal_status: HAL status
  * @retval USB status
  */
USBH_StatusTypeDef USBH_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
  USBH_StatusTypeDef usb_status = USBH_OK;

  switch (hal_status)
  {
    case HAL_OK :
      usb_status = USBH_OK;
    break;
    case HAL_ERROR :
      usb_status = USBH_FAIL;
    break;
    case HAL_BUSY :
      usb_status = USBH_BUSY;
    break;
    case HAL_TIMEOUT :
      usb_status = USBH_FAIL;
    break;
    default :
      usb_status = USBH_FAIL;
    break;
  }
  return usb_status;
}

