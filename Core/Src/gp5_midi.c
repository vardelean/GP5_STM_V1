/**
  ******************************************************************************
  * @file    gp5_midi.c
  * @author  Custom Implementation
  * @brief   GP-5 Pedal MIDI Communication Implementation (Simplified)
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gp5_midi.h"
#include "midi_manager.h"
#include "preset_buttons.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DEBUG_VERBOSE 0  /* Set to 1 for detailed logging, 0 for production */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Preset tracking variables */
static uint8_t currentPresetNumber = 0;
static bool presetNumberValid = false;
static bool lastPresetChangeWasByUs = false;  /* Track if we sent last CC#0 */

/* Current patch state tracking - what's currently ON/OFF on the GP-5 */
static PatchInfo_t currentPatchState;
static bool currentStateValid = false;

/* External variables --------------------------------------------------------*/
extern uint8_t midi_device_connected;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize GP-5 MIDI communication
  * @retval None
  */
void GP5_MIDI_Init(void)
{
  printf("GP-5 MIDI initialized\r\n");
  
  /* Initialize state tracking */
  presetNumberValid = false;
  currentStateValid = false;
  
  /* Initialize current patch state to all OFF */
  memset(&currentPatchState, 0, sizeof(currentPatchState));
}

/**
  * @brief  Request initial preset number from GP-5 (call after enumeration)
  * @retval None
  */
void GP5_MIDI_RequestInitialPreset(void)
{
  printf("[GP-5] Requesting initial preset...\r\n");
  GP5_MIDI_RequestPresetNumber();
}

/**
  * @brief  Request preset number from GP-5
  * @retval None
  */
void GP5_MIDI_RequestPresetNumber(void)
{
  if (!midi_device_connected)
  {
    printf("[GP-5] Cannot request preset - no MIDI device connected\r\n");
    return;
  }
  
  /* GP-5 Preset Number Request SysEx: F0 00 07 00 01 00 00 00 02 01 02 04 03 F7 */
  uint8_t preset_request[] = {0xF0, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x02, 0x04, 0x03, 0xF7};
  
  MIDI_Manager_SendSysEx(preset_request, sizeof(preset_request));
}

/**
  * @brief  Parse preset number response (18 bytes)
  * @param  data: Pointer to clean MIDI data
  * @param  len: Length of data
  * @param  preset: Pointer to store preset number (0-99)
  * @retval true if valid preset number response, false otherwise
  */
bool GP5_MIDI_ParsePresetNumber(uint8_t *data, uint16_t len, uint8_t *preset)
{
  /* GP-5 response format: F0 0E 0A 00 01 00 00 00 04 01 02 04 03 XX YY 00 00 F7 (18 bytes) */
  /* Preset number is in bytes 13 and 14: preset = (data[13] << 4) | data[14] */
  
  if (len != 18)
    return false;
    
  if (data[0] != 0xF0 || data[len-1] != 0xF7)
  {
    printf("[ParsePreset] Missing F0 or F7\r\n");
    return false;
  }
    
  /* Check key bytes to identify preset number response */
  if (data[3] == 0 && data[4] == 1 && data[8] == 4 && 
      data[9] == 1 && data[10] == 2 && data[11] == 4 && data[15] == 0)
  {
    /* Preset number is in bytes 13 and 14 */
    *preset = (data[13] << 4) | data[14];
    
    if (*preset > 99)
    {
      printf("[ParsePreset] Invalid preset: %d\r\n", *preset);
      return false;  /* Invalid preset number */
    }
    
    printf("[ParsePreset] SUCCESS! Extracted preset: %d (from bytes 13=0x%02X, 14=0x%02X)\r\n", 
           *preset, data[13], data[14]);
    return true;
  }
  
  printf("[ParsePreset] Pattern mismatch\r\n");
  return false;
}

/**
  * @brief  Detect preset change ACK (22 bytes)
  * @param  data: Pointer to clean MIDI data
  * @param  len: Length of data
  * @retval true if preset change ACK detected, false otherwise
  */
bool GP5_MIDI_IsPresetChangeACK(uint8_t *data, uint16_t len)
{
  /* Expected format based on actual GP-5 response: F0 0E 02 00 01 00 ... F7 (22 bytes total) */
  
  if (len != 22)
    return false;
    
  if (data[0] != 0xF0 || data[len-1] != 0xF7)
    return false;
    
  /* Check header for preset change ACK: F0 0E 02 00 01 00 */
  if (data[1] == 0x0E && data[2] == 0x02 && data[3] == 0x00 &&
      data[4] == 0x01 && data[5] == 0x00)
    return true;
  
  return false;
}

/**
  * @brief  Request patch info from GP-5
  * @retval None
  */
void GP5_MIDI_RequestPatchInfo(void)
{
  if (!midi_device_connected)
  {
    printf("[GP-5] Cannot request patch info - no MIDI device connected\r\n");
    return;
  }
  
  /* GP-5 Patch Info Request SysEx: F0 00 01 0C 00 10 00 F7 */
  uint8_t patch_request[] = {0xF0, 0x00, 0x01, 0x0C, 0x00, 0x10, 0x00, 0xF7};
  
  if (DEBUG_VERBOSE)
    printf("[GP-5] Sending patch info request\r\n");
    
  MIDI_Manager_SendSysEx(patch_request, sizeof(patch_request));
}

/**
  * @brief  Parse patch info response (4th message, 48 bytes)
  * @param  data: Pointer to clean MIDI data
  * @param  len: Length of data
  * @param  patchBitmap: Pointer to store 32-bit patch bitmap
  * @retval true if valid patch info (4th message), false otherwise
  */
bool GP5_MIDI_ParsePatchInfo(uint8_t *data, uint16_t len, uint32_t *patchBitmap)
{
  /* Expected format: F0 00 01 0C 00 10 03 ... F7 (48 bytes total) */
  /* Byte 6 = 0x03 indicates 4th message (message index) */
  
  if (len != 48)
    return false;
    
  if (data[0] != 0xF0 || data[len-1] != 0xF7)
    return false;
    
  /* Check header: 00 01 0C 00 10 03 */
  if (data[1] != 0x00 || data[2] != 0x01 || data[3] != 0x0C ||
      data[4] != 0x00 || data[5] != 0x10 || data[6] != 0x03)
    return false;
  
  /* Patch bitmap extraction - bits 8-17 (10 patches) */
  /* Based on observed data, construct 10-bit bitmap */
  
  /* Extract relevant bytes that contain patch states */
  /* This is a simplified extraction - adjust based on actual GP-5 format */
  uint32_t bitmap = 0;
  
  /* Parse patch states from response (exact format depends on GP-5 protocol) */
  /* For now, extract from observed byte positions */
  for (int i = 0; i < 10; i++)
  {
    /* Adjust byte positions based on actual GP-5 response format */
    if (data[7 + i] != 0)  /* Example: check bytes 7-16 */
      bitmap |= (1 << (8 + i));  /* Set bit 8-17 */
  }
  
  *patchBitmap = bitmap;
  return true;
}

/**
  * @brief  Set patch on/off state
  * @param  patchCC: CC number (48-57)
  * @param  turnOn: true to turn on, false to turn off
  * @retval None
  */
void GP5_MIDI_SetPatchState(uint8_t patchCC, bool turnOn)
{
  if (!midi_device_connected)
  {
    printf("[GP-5] Cannot set patch - no MIDI device connected\r\n");
    return;
  }
  
  /* Send CC message: CC value 127 = ON, 0 = OFF */
  uint8_t value = turnOn ? 127 : 0;
  
  if (DEBUG_VERBOSE)
    printf("[GP-5] CC#%d = %d\r\n", patchCC, value);
    
  MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, patchCC, value);
  
  /* Small delay between CC commands */
  HAL_Delay(10);
}

/**
  * @brief  Notify that we are sending a preset change command
  * @retval None
  */
void GP5_MIDI_NotifyPresetChangeSent(void)
{
  lastPresetChangeWasByUs = true;
}

/**
  * @brief  Process received MIDI messages from GP-5
  * @param  data: Pointer to USB-MIDI packet data
  * @param  length: Length of data in bytes
  * @retval None
  */
void GP5_MIDI_ProcessReceivedData(uint8_t *data, uint16_t length)
{
  if (length == 0)
    return;
  
  /* USB-MIDI packets are 4 bytes: [CIN+CN][MIDI1][MIDI2][MIDI3] */
  /* Extract clean MIDI data by removing USB-MIDI headers */
  
  static uint8_t sysex_buffer[256];
  static uint16_t sysex_index = 0;
  static bool in_sysex = false;
  
  for (uint16_t i = 0; i < length; i += 4)
  {
    uint8_t cin = data[i] & 0x0F;  /* Code Index Number is in LOWER nibble */
    
    /* Handle SysEx Start (CIN = 0x04) */
    if (cin == 0x04)
    {
      if (!in_sysex)
      {
        in_sysex = true;
        sysex_index = 0;
      }
      sysex_buffer[sysex_index++] = data[i+1];  /* F0 or continue */
      sysex_buffer[sysex_index++] = data[i+2];
      sysex_buffer[sysex_index++] = data[i+3];
    }
    /* Handle SysEx End (CIN = 0x05, 0x06, 0x07) */
    else if ((cin == 0x05 || cin == 0x06 || cin == 0x07) && in_sysex)
    {
      /* CIN 0x05: 1 byte follows (F7) */
      /* CIN 0x06: 2 bytes follow (XX F7) */
      /* CIN 0x07: 3 bytes follow (XX XX F7) */
      uint8_t bytes_to_copy = (cin == 0x05) ? 1 : (cin == 0x06) ? 2 : 3;
      
      for (uint8_t j = 0; j < bytes_to_copy; j++)
      {
        sysex_buffer[sysex_index++] = data[i+1+j];
      }
      
      in_sysex = false;
      
      /* Process complete SysEx message */
      uint8_t preset;
      uint32_t patchBitmap;
      
      /* Check for Preset Number Response */
      if (GP5_MIDI_ParsePresetNumber(sysex_buffer, sysex_index, &preset))
      {
        currentPresetNumber = preset;
        presetNumberValid = true;
        printf("[GP-5] Preset Number: %d\r\n", preset);
        
        /* Notify preset button handler */
        PresetButtons_SetCurrentPreset(preset);
      }
      /* Check for Preset Change ACK */
      else if (GP5_MIDI_IsPresetChangeACK(sysex_buffer, sysex_index))
      {
        /* Notify preset button handler */
        PresetButtons_OnPresetChangeACK(lastPresetChangeWasByUs);
        lastPresetChangeWasByUs = false;  /* Reset flag */
      }
      /* Check for Patch Info Response */
      else if (GP5_MIDI_ParsePatchInfo(sysex_buffer, sysex_index, &patchBitmap))
      {
        printf("[GP-5] Patch Info: 0x%08lX\r\n", patchBitmap);
        
        /* Update current state */
        currentPatchState.patchNR  = (patchBitmap >> 8) & 1;
        currentPatchState.patchPRE = (patchBitmap >> 9) & 1;
        currentPatchState.patchDST = (patchBitmap >> 10) & 1;
        currentPatchState.patchNS  = (patchBitmap >> 11) & 1;
        currentPatchState.patchAMP = (patchBitmap >> 12) & 1;
        currentPatchState.patchCAB = (patchBitmap >> 13) & 1;
        currentPatchState.patchEQ  = (patchBitmap >> 14) & 1;
        currentPatchState.patchMOD = (patchBitmap >> 15) & 1;
        currentPatchState.patchDLY = (patchBitmap >> 16) & 1;
        currentPatchState.patchRVB = (patchBitmap >> 17) & 1;
        currentStateValid = true;
        
        printf("[GP-5] Patches: NR=%d PRE=%d DST=%d NS=%d AMP=%d CAB=%d EQ=%d MOD=%d DLY=%d RVB=%d\r\n",
               currentPatchState.patchNR, currentPatchState.patchPRE,
               currentPatchState.patchDST, currentPatchState.patchNS,
               currentPatchState.patchAMP, currentPatchState.patchCAB,
               currentPatchState.patchEQ, currentPatchState.patchMOD,
               currentPatchState.patchDLY, currentPatchState.patchRVB);
      }
      else if (DEBUG_VERBOSE)
      {
        printf("[GP-5] Unknown SysEx (%d bytes)\r\n", sysex_index);
      }
      
      sysex_index = 0;
    }
  }
}
