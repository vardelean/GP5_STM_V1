/**
  ******************************************************************************
  * @file    gp5_midi.c
  * @author  Custom Implementation
  * @brief   GP-5 Pedal MIDI Communication Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gp5_midi.h"
#include "midi_manager.h"
#include "led_controller.h"
#include "flash_storage.h"
#include "scene_manager.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t sysex_data[256];
  uint16_t sysex_length;
} SysExConfig_t;

/* Private define ------------------------------------------------------------*/
#define DEBUG_VERBOSE 0  /* Set to 1 for detailed logging, 0 for production */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static SysExConfig_t button_sysex_config[BTN_COUNT];
static const char *button_names[] = {"btnUp", "btnDown", "btnScene1", "btnScene2", "btnScene3", "btnTap"};

/* Preset tracking variables */
static uint8_t currentPresetNumber = 0;
static bool presetNumberValid = false;
static uint8_t activeSceneNumber = 1;  /* Default to Scene 1 */

/* Scene recall cooldown */
static uint32_t lastSceneRecallCompleteTime = 0;
#define SCENE_RECALL_COOLDOWN_MS 300  /* Ignore scene buttons for 300ms after recall */

/* Scene save state */
static bool awaitingSaveConfirmation = false;
static uint8_t pendingSceneSave = 0;  /* 0 = none, 1-3 = scene number to save */

/* External variables --------------------------------------------------------*/
extern uint8_t midi_device_connected;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize GP-5 MIDI messages for all buttons
  * @retval None
  */
void GP5_MIDI_Init(void)
{
  /* Clear all SysEx configurations */
  memset(button_sysex_config, 0, sizeof(button_sysex_config));
  
  /* Button Up and Down use CC messages (configured separately in handler) */
  button_sysex_config[BTN_UP].sysex_length = 0;
  button_sysex_config[BTN_DOWN].sysex_length = 0;
  
  /* Scene buttons - Configure GP-5 specific SysEx messages */
  /* Example SysEx format for GP-5 scene change: F0 00 01 XX F7 */
  button_sysex_config[BTN_SCENE1].sysex_data[0] = 0xF0;  /* SysEx Start */
  button_sysex_config[BTN_SCENE1].sysex_data[1] = 0x00;  /* Manufacturer ID placeholder */
  button_sysex_config[BTN_SCENE1].sysex_data[2] = 0x01;  /* Command: Scene change */
  button_sysex_config[BTN_SCENE1].sysex_data[3] = 0x01;  /* Scene 1 */
  button_sysex_config[BTN_SCENE1].sysex_data[4] = 0xF7;  /* SysEx End */
  button_sysex_config[BTN_SCENE1].sysex_length = 5;
  
  button_sysex_config[BTN_SCENE2].sysex_data[0] = 0xF0;
  button_sysex_config[BTN_SCENE2].sysex_data[1] = 0x00;
  button_sysex_config[BTN_SCENE2].sysex_data[2] = 0x01;
  button_sysex_config[BTN_SCENE2].sysex_data[3] = 0x02;  /* Scene 2 */
  button_sysex_config[BTN_SCENE2].sysex_data[4] = 0xF7;
  button_sysex_config[BTN_SCENE2].sysex_length = 5;
  
  button_sysex_config[BTN_SCENE3].sysex_data[0] = 0xF0;
  button_sysex_config[BTN_SCENE3].sysex_data[1] = 0x00;
  button_sysex_config[BTN_SCENE3].sysex_data[2] = 0x01;
  button_sysex_config[BTN_SCENE3].sysex_data[3] = 0x03;  /* Scene 3 */
  button_sysex_config[BTN_SCENE3].sysex_data[4] = 0xF7;
  button_sysex_config[BTN_SCENE3].sysex_length = 5;
  
  /* Tap button - uses CC message (handled in handler) */
  button_sysex_config[BTN_TAP].sysex_length = 0;
  
  printf("GP-5 MIDI initialized\r\n");
  
  /* Request initial preset number (will be sent when GP-5 connects) */
  presetNumberValid = false;
  activeSceneNumber = 1;
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
  * @brief  Test function: Save a test scene
  * @param  preset: Preset number
  * @param  sceneNum: Scene number (1-3)
  * @retval None
  */
void GP5_MIDI_SaveTestScene(uint8_t preset, uint8_t sceneNum)
{
  /* Create a test patch bitmap with some patches ON */
  /* Example: NR=1, PRE=1, DST=1, AMP=1 */
  uint32_t testBitmap = (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11);
  
  printf("[TEST] Saving test scene %d for preset %d (bitmap: 0x%08lX)\r\n", 
         sceneNum, preset, testBitmap);
  
  SceneManager_SaveScene(preset, sceneNum, testBitmap);
  
  printf("[TEST] Scene saved. Press btnScene%d to recall.\r\n", sceneNum);
}

/**
  * @brief  Handle button events and send appropriate GP-5 MIDI messages
  * @param  button: Button ID
  * @param  event: Button event type
  * @retval None
  */
void GP5_MIDI_HandleButtonEvent(ButtonID_t button, ButtonEvent_t event)
{
  if (button >= BTN_COUNT)
    return;
    
  if (event == BTN_EVENT_SHORT_PRESS)
  {
    /* Handle Scene 1 button - recall preset defaults via CC#0 */
    if (button == BTN_SCENE1)
    {
      #if DEBUG_VERBOSE
      printf("[Scene] Recalling Scene 1 (preset defaults) for preset %d...\r\n", currentPresetNumber);
      #endif
      
      /* Send CC#0 with current preset number to reload defaults */
      MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, 0, currentPresetNumber);
      
      /* Update LED */
      LED_SetScene(LED_SCENE1);
      activeSceneNumber = 1;
      
      return;
    }
    
    /* Handle Scene 2 and 3 button short press - recall saved scene */
    if (button == BTN_SCENE2 || button == BTN_SCENE3)
    {
      uint8_t sceneNum = (button - BTN_SCENE1) + 1;  /* 2 or 3 */
      
      /* Check if scene is programmed */
      if (!SceneManager_IsSceneProgrammed(currentPresetNumber, sceneNum))
      {
        printf("[Scene] Scene %d not programmed for preset %d\r\n", sceneNum, currentPresetNumber);
        return;  /* Do nothing */
      }
      
      /* Check cooldown period - ignore rapid re-presses */
      if (lastSceneRecallCompleteTime != 0)
      {
        uint32_t timeSinceLastRecall = HAL_GetTick() - lastSceneRecallCompleteTime;
        if (timeSinceLastRecall < SCENE_RECALL_COOLDOWN_MS)
        {
          printf("[Scene] Cooldown active (%lu ms remaining)\r\n", 
                 SCENE_RECALL_COOLDOWN_MS - timeSinceLastRecall);
          return;
        }
      }
      
      #if DEBUG_VERBOSE
      printf("[Scene] Recalling Scene %d...\r\n", sceneNum);
      #endif
      
      /* Get saved scene patches */
      PatchInfo_t savedPatches;
      SceneManager_GetScenePatches(currentPresetNumber, sceneNum, &savedPatches);
      
      /* Send ALL CC commands immediately (GP-5 queues them) - NO DELAYS! */
      GP5_MIDI_SetPatchState(GP5_CC_NR, savedPatches.patchNR);
      GP5_MIDI_SetPatchState(GP5_CC_PRE, savedPatches.patchPRE);
      GP5_MIDI_SetPatchState(GP5_CC_DST, savedPatches.patchDST);
      GP5_MIDI_SetPatchState(GP5_CC_NS, savedPatches.patchNS);
      GP5_MIDI_SetPatchState(GP5_CC_AMP, savedPatches.patchAMP);
      GP5_MIDI_SetPatchState(GP5_CC_CAB, savedPatches.patchCAB);
      GP5_MIDI_SetPatchState(GP5_CC_EQ, savedPatches.patchEQ);
      GP5_MIDI_SetPatchState(GP5_CC_MOD, savedPatches.patchMOD);
      GP5_MIDI_SetPatchState(GP5_CC_DLY, savedPatches.patchDLY);
      GP5_MIDI_SetPatchState(GP5_CC_RVB, savedPatches.patchRVB);
      
      /* Force app refresh with CTL screen flicker */
      MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_CTL_SCREEN, 127);  /* Enter CTL */
      MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_CTL_SCREEN, 0);    /* Exit CTL */
      
      #if DEBUG_VERBOSE
      printf("[Scene] Scene %d recalled successfully\r\n", sceneNum);
      #endif      
      /* Update scene LED */
      if (sceneNum == 2) {
        LED_SetScene(LED_SCENE2);
      } else if (sceneNum == 3) {
        LED_SetScene(LED_SCENE3);
      }
      
      activeSceneNumber = sceneNum;
      lastSceneRecallCompleteTime = HAL_GetTick();  /* Start cooldown */
      
      return;
    }
    
    /* Send GP-5 specific MIDI messages */
    if (midi_device_connected)
    {
      if (button == BTN_UP)
      {
        /* Patch Up - CC#25 */
        MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_PATCH_UP, 127);
      }
      else if (button == BTN_DOWN)
      {
        /* Patch Down - CC#24 */
        MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_PATCH_DOWN, 127);
      }
      else if (button == BTN_TAP)
      {
        /* Tap Tempo - CC#64 */
        MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_TAP_TEMPO, 127);
      }
      else if (button_sysex_config[button].sysex_length > 0)
      {
        /* Scene change via SysEx */
        printf("Sending GP-5 SysEx for %s\r\n", button_names[button]);
        MIDI_Manager_SendSysEx(button_sysex_config[button].sysex_data,
                              button_sysex_config[button].sysex_length);
      }
    }
  }
  else if (event == BTN_EVENT_LONG_PRESS)
  {
    /* Handle long press on Scene 2 and 3 buttons ONLY - save current state */
    if (button == BTN_SCENE2 || button == BTN_SCENE3)
    {
      uint8_t sceneNum = (button - BTN_SCENE1) + 1;  /* 2 or 3 */
      
      printf("[Scene] Saving Scene %d for preset %d\r\n", sceneNum, currentPresetNumber);
      
      /* Set flag and request patch info */
      pendingSceneSave = sceneNum;
      awaitingSaveConfirmation = true;
      
      /* Flash the LED quickly to show we detected the long press */
      if (sceneNum == 2) LED_SetScene(LED_SCENE2);
      else if (sceneNum == 3) LED_SetScene(LED_SCENE3);
      HAL_Delay(50);
      LED_AllOff();
      HAL_Delay(50);
      if (sceneNum == 2) LED_SetScene(LED_SCENE2);
      else if (sceneNum == 3) LED_SetScene(LED_SCENE3);
      HAL_Delay(50);
      LED_AllOff();
      
      /* Now request patch info (response will trigger the save) */
      GP5_MIDI_RequestPatchInfo();
    }
    else if (button != BTN_SCENE1)  /* Scene1 long press ignored */
    {
      /* Old test code for non-scene buttons */
      uint8_t flash_data[4] = {0xAA, 0xBB, 0xCC, (uint8_t)button};
      FlashStorage_StatusTypeDef status = FlashStorage_WriteData(button, flash_data);
      
      if (status == FLASH_STORAGE_OK)
      {
        printf("Flash write successful for button %d\r\n", button);
      }
      else
      {
        printf("Flash write failed\r\n");
      }
    }
  }
  else if (event == BTN_EVENT_EXTRA_LONG_PRESS)
  {
    /* Handle extra long press on Scene 2 and 3 buttons ONLY - delete scene */
    if (button == BTN_SCENE2 || button == BTN_SCENE3)
    {
      uint8_t sceneNum = (button - BTN_SCENE1) + 1;  /* 2 or 3 */
      
      printf("[Scene] Deleting Scene %d for preset %d\r\n", sceneNum, currentPresetNumber);
      
      /* Fast blink LED to show delete mode */
      for (int i = 0; i < 5; i++)
      {
        if (sceneNum == 2) LED_SetScene(LED_SCENE2);
        else if (sceneNum == 3) LED_SetScene(LED_SCENE3);
        HAL_Delay(50);
        LED_AllOff();
        HAL_Delay(50);
      }
      
      /* Delete the scene */
      SceneManager_DeleteScene(currentPresetNumber, sceneNum);
      printf("[Scene] Scene %d deleted\r\n", sceneNum);
      
      /* After deleting Scene2 or Scene3, always return to Scene 1 (preset defaults) */
      MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, 0, currentPresetNumber);
      
      /* Turn on Scene 1 LED */
      LED_SetScene(LED_SCENE1);
      activeSceneNumber = 1;
    }
    /* Scene1 extra-long press is ignored - cannot delete defaults */
  }
  else if (event == BTN_EVENT_RELEASED)
  {
    /* Button released - no action needed */
  }
}

/**
  * @brief  Process received MIDI messages from GP-5
  * @param  data: Pointer to USB-MIDI packet data
  * @param  length: Length of data in bytes
  * @retval None
  */
void GP5_MIDI_ProcessReceivedData(uint8_t *data, uint16_t length)
{
  uint8_t clean_midi[256];  /* Buffer for extracted MIDI data */
  uint16_t clean_length = 0;
  
  /* Extract clean MIDI data from USB-MIDI packets */
  for (uint16_t i = 0; i < length; i += 4)
  {
    if (i + 3 >= length)
      break;
      
    uint8_t cin = data[i] & 0x0F;  /* Code Index Number */
    
    /* Extract MIDI bytes based on CIN */
    if (cin >= 0x04 && cin <= 0x07)  /* SysEx */
    {
      /* CIN 0x04 = SysEx starts or continues (3 bytes)
         CIN 0x05 = SysEx ends with 1 byte, or 1-byte system common
         CIN 0x06 = SysEx ends with 2 bytes
         CIN 0x07 = SysEx ends with 3 bytes */
      
      if (cin == 0x04 || cin == 0x07)  /* 3 data bytes */
      {
        clean_midi[clean_length++] = data[i + 1];
        clean_midi[clean_length++] = data[i + 2];
        clean_midi[clean_length++] = data[i + 3];
      }
      else if (cin == 0x06)  /* 2 data bytes */
      {
        clean_midi[clean_length++] = data[i + 1];
        clean_midi[clean_length++] = data[i + 2];
      }
      else if (cin == 0x05)  /* 1 data byte */
      {
        clean_midi[clean_length++] = data[i + 1];
      }
    }
    else if (cin >= 0x08 && cin <= 0x0E)  /* Channel messages */
    {
      /* 3-byte messages: Note On/Off, CC, Poly Aftertouch, Pitch Bend */
      if (cin == 0x08 || cin == 0x09 || cin == 0x0B || cin == 0x0E)
      {
        clean_midi[clean_length++] = data[i + 1];
        clean_midi[clean_length++] = data[i + 2];
        clean_midi[clean_length++] = data[i + 3];
      }
      /* 2-byte messages: Program Change, Channel Aftertouch */
      else if (cin == 0x0C || cin == 0x0D)
      {
        clean_midi[clean_length++] = data[i + 1];
        clean_midi[clean_length++] = data[i + 2];
      }
    }
  }
  
  /* Decode clean MIDI messages (minimal output) */
  if (clean_length > 0)
  {
    if (clean_midi[0] == 0xF0)  /* SysEx */
    {
      /* Check for preset change ACK */
      if (GP5_MIDI_IsPresetChangeACK(clean_midi, clean_length))
      {
        #if DEBUG_VERBOSE
        printf("[GP-5] Preset change ACK\r\n");
        #endif
        
        /* Automatically request preset number */
        GP5_MIDI_RequestPresetNumber();
      }
      /* Check for preset number response */
      else if (clean_length == 18 && clean_midi[3] == 0x00 && clean_midi[4] == 0x01)
      {
        uint8_t preset;
        if (GP5_MIDI_ParsePresetNumber(clean_midi, clean_length, &preset))
        {
          printf("[GP-5] Preset %d\r\n", preset);
          
          /* Update current preset */
          currentPresetNumber = preset;
          presetNumberValid = true;
          
          /* Scene 1 is always "available" (represents preset defaults) */
          /* Turn ON ledScene1 after any preset change */
          LED_SetScene(LED_SCENE1);
          activeSceneNumber = 1;
        }
      }
      /* Check for patch info (4th message) - used for scene save */
      else if (clean_length == 48)
      {
        uint32_t patchBitmap;
        if (GP5_MIDI_ParsePatchInfo(clean_midi, clean_length, &patchBitmap))
        {
          #if DEBUG_VERBOSE
          printf("[GP-5] Patch info: 0x%08lX\r\n", patchBitmap);
          #endif
          
          /* Check if we're waiting to save a scene */
          if (awaitingSaveConfirmation && pendingSceneSave > 0)
          {
            /* Save the scene */
            SceneManager_SaveScene(currentPresetNumber, pendingSceneSave, patchBitmap);
            printf("[Scene] Scene %d saved\r\n", pendingSceneSave);
            
            /* Turn on the scene LED to confirm (only Scene 2 or 3) */
            if (pendingSceneSave == 2) {
              LED_SetScene(LED_SCENE2);
              activeSceneNumber = 2;
            } else if (pendingSceneSave == 3) {
              LED_SetScene(LED_SCENE3);
              activeSceneNumber = 3;
            }
            
            /* Clear save state */
            awaitingSaveConfirmation = false;
            pendingSceneSave = 0;
          }
        }
      }
    }
    else if ((clean_midi[0] & 0xF0) == 0xB0)  /* Control Change */
    {
      /* CC messages - silent unless debugging */
      #if DEBUG_VERBOSE
      uint8_t cc_number = clean_midi[1];
      uint8_t cc_value = clean_midi[2];
      printf("[MIDI] CC#%d = %d\r\n", cc_number, cc_value);
      #endif
    }
  }
}

/**
  * @brief  Request preset number from GP-5
  * @retval None
  */
void GP5_MIDI_RequestPresetNumber(void)
{
  /* SysEx: F0 00 07 00 01 00 00 00 02 01 02 04 03 F7 */
  uint8_t reqPstNum[] = {0xF0, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 
                         0x02, 0x01, 0x02, 0x04, 0x03, 0xF7};
  
  MIDI_Manager_SendSysEx(reqPstNum, sizeof(reqPstNum));
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
  /* Check length and signature bytes */
  if (len == 18 && 
      data[3] == 0x00 && data[4] == 0x01 && 
      data[8] == 0x04 && data[9] == 0x01 && 
      data[10] == 0x02 && data[11] == 0x04 && 
      data[15] == 0x00)
  {
    /* Extract preset number from bytes 13-14 */
    *preset = (data[13] << 4) | data[14];
    return true;
  }
  
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
  /* Check length and signature bytes */
  if (len == 22 && 
      data[3] == 0x00 && data[4] == 0x01 && 
      data[8] == 0x06 && data[9] == 0x01 && 
      data[10] == 0x02 && data[11] == 0x01 && 
      data[12] == 0x0B)
  {
    return true;
  }
  
  return false;
}

/**
  * @brief  Request patch info from GP-5
  * @retval None
  */
void GP5_MIDI_RequestPatchInfo(void)
{
  /* SysEx: F0 00 09 00 01 00 00 00 02 01 02 04 01 F7 */
  uint8_t reqPatchInfo[] = {0xF0, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 
                            0x02, 0x01, 0x02, 0x04, 0x01, 0xF7};
  
  MIDI_Manager_SendSysEx(reqPatchInfo, sizeof(reqPatchInfo));
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
  /* Check for 4th message: len=48, data[3]=0x01, data[4]=0x09, data[5]=0x00, data[6]=0x03 */
  if (len == 48 && 
      data[3] == 0x01 && data[4] == 0x09 && 
      data[5] == 0x00 && data[6] == 0x03)
  {
    /* Extract patch bitmap from bytes 35-38 */
    *patchBitmap = ((uint32_t)data[38] << 24) | 
                   ((uint32_t)data[37] << 16) | 
                   ((uint32_t)data[36] << 8) | 
                   ((uint32_t)data[35]);
    return true;
  }
  
  return false;
}

/**
  * @brief  Set patch on/off state
  * @param  patchCC: CC number (48-57)
  * @param  turnOn: true to turn on, false to turn off
  * @retval None
  */
void GP5_MIDI_SetPatchState(uint8_t patchCC, bool turnOn)
{
  /* CC value: 0-63 = OFF, 64-127 = ON */
  uint8_t value = turnOn ? 127 : 0;
  
  #if DEBUG_VERBOSE
  printf("[GP-5] CC#%d = %s\r\n", patchCC, turnOn ? "ON" : "OFF");
  #endif
  
  MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, patchCC, value);
}

/* Private functions ---------------------------------------------------------*/
