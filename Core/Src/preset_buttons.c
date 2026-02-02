/**
  ******************************************************************************
  * @file    preset_buttons.c
  * @author  Custom Implementation
  * @brief   Preset and Bank Button Handler with Flash Storage
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "preset_buttons.h"
#include "midi_manager.h"
#include "gp5_midi.h"
#include "display.h"
#include "flash_storage.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  BTN_STATE_IDLE = 0,
  BTN_STATE_PRESSED,
  BTN_STATE_HELD_SAVE,       /* Held 2+ seconds (save mode) */
  BTN_STATE_HELD_CLEAR,      /* Held 5+ seconds (clear mode) */
  BTN_STATE_DEBOUNCING
} ButtonState_t;

/* Private define ------------------------------------------------------------*/
#define DEBOUNCE_TIME_MS    20
#define PRESET_RETRY_COUNT  3
#define PRESET_RETRY_DELAY_MS  50

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t currentBank = 0;              /* Current STM32 bank (0-15) */
static uint8_t temporaryBank = 0;            /* Temporary bank when bank up/down pressed */
static uint8_t currentButton = 0;            /* Current button (0-4) */
static uint8_t currentGP5Preset = 0;         /* Current GP-5 preset (0-99) */
static bool currentPresetValid = false;      /* Is current GP-5 preset known? */
static bool isBankInverted = false;          /* Is bank display inverted? */

static ButtonState_t buttonState = BTN_STATE_IDLE;
static uint16_t pressedButton = 0;           /* Which button is pressed */
static uint8_t pressedButtonNum = 0;         /* Preset button number (0-4) */
static uint32_t buttonPressTime = 0;         /* When button was pressed */
static bool isTapTempoButton = false;        /* Is Tap Tempo button pressed? */
static bool isCtlButton = false;             /* Is CTL button pressed? */
static uint32_t debounceStartTime = 0;       /* When debounce started */
static bool commandSentThisPress = false;    /* Did we send a command? */
static bool saveActionTriggered = false;     /* Did we trigger save action? */
static bool clearActionTriggered = false;    /* Did we trigger clear action? */
static bool tunerActionTriggered = false;    /* Did we trigger tuner toggle? */

/* Startup preset recall */
static uint8_t presetRecallRetryCount = 0;
static uint32_t lastPresetRecallTime = 0;
static bool startupPresetRecallPending = false;
static bool tunerOn = false;             /* Tuner state */

/* Tap tempo variables */
static uint32_t lastTapTime = 0;         /* Last tap tempo press time */
static uint32_t tempoMs = 0;             /* Tempo in milliseconds */
#define TAP_TEMPO_MIN_MS    20          /* Fastest tempo (20ms) */
#define TAP_TEMPO_MAX_MS    1000        /* Slowest tempo (1000ms) */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void HandlePresetButton(uint8_t buttonNum);
static void HandleBankUpButton(void);
static void HandleBankDownButton(void);
static uint8_t CalculatePresetIndex(uint8_t bank, uint8_t button);
static void UpdateDisplayForCurrentPreset(void);
static void SendPresetWithRetry(uint8_t gp5Preset);
static void SendTapTempoSysEx(uint32_t tempoMs);
static uint8_t CalculateCRC8(const uint8_t* data, uint32_t length);
static void FloatToHexBytes(float value, uint8_t* bytes);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize preset button handler
  * @retval None
  */
void PresetButtons_Init(void)
{
  /* Initialize Flash storage first */
  if (FlashStorage_Init() != 0)
  {
    printf("[PresetButtons] ERROR: Failed to initialize Flash storage\r\n");
    return;
  }
  
  /* Retrieve saved current preset index from Flash */
  uint8_t savedIndex = FlashStorage_GetCurrentPresetIndex();
  FlashStorage_ExtractBankButton(savedIndex, &currentBank, &currentButton);
  
  printf("[PresetButtons] Loaded saved preset: Bank %d, Button %d (Index %d)\r\n", 
         currentBank + 1, currentButton + 1, savedIndex);
  
  temporaryBank = currentBank;
  currentGP5Preset = 0;
  currentPresetValid = false;
  isBankInverted = false;
  buttonState = BTN_STATE_IDLE;
  pressedButton = 0;
  pressedButtonNum = 0;
  buttonPressTime = 0;
  isTapTempoButton = false;
  isCtlButton = false;
  commandSentThisPress = false;
  saveActionTriggered = false;
  clearActionTriggered = false;
  startupPresetRecallPending = true;  /* Will be handled after GP-5 connection */
  presetRecallRetryCount = 0;
  
  /* Enable and configure NVIC for EXTI interrupts */
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
  
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
  
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
  
  /* Update display with saved preset info */
  UpdateDisplayForCurrentPreset();
  
  printf("[PresetButtons] Initialized\r\n");
}

/**
  * @brief  Request startup preset recall (call after GP-5 connection established)
  * @retval None
  */
void PresetButtons_RequestStartupPresetRecall(void)
{
  if (!startupPresetRecallPending)
    return;
  
  /* Get stored GP-5 preset for current STM32 preset */
  uint8_t presetIndex = CalculatePresetIndex(currentBank, currentButton);
  uint8_t storedGP5Preset = FlashStorage_GetPreset(presetIndex);
  
  if (storedGP5Preset == 0xFF)
  {
    printf("[PresetButtons] No preset stored for Bank %d Button %d, requesting current GP-5 preset\r\n",
           currentBank + 1, currentButton + 1);
    GP5_MIDI_RequestPresetNumber();
    startupPresetRecallPending = false;
    return;
  }
  
  printf("[PresetButtons] Recalling stored preset: GP-5 #%d\r\n", storedGP5Preset);
  SendPresetWithRetry(storedGP5Preset);
  startupPresetRecallPending = false;
}

/**
  * @brief  GPIO EXTI callback for button interrupts
  * @param  GPIO_Pin: Pin number that triggered the interrupt
  * @retval None
  */
void PresetButtons_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  GPIO_PinState pinState = HAL_GPIO_ReadPin(GPIOB, GPIO_Pin);
  
  /* Falling edge (button pressed) - active low with pullup */
  if (pinState == GPIO_PIN_RESET)
  {
    /* Only process if no button is currently pressed */
    if (buttonState == BTN_STATE_IDLE)
    {
      buttonState = BTN_STATE_PRESSED;
      pressedButton = GPIO_Pin;
      buttonPressTime = HAL_GetTick();
      commandSentThisPress = false;
      saveActionTriggered = false;
      clearActionTriggered = false;
      tunerActionTriggered = false;
      
      /* Handle preset buttons immediately (will send CC if not held) */
      if (GPIO_Pin == btnPst0_Pin)
      {
        pressedButtonNum = 0;
        HandlePresetButton(0);
      }
      else if (GPIO_Pin == btnPst1_Pin)
      {
        pressedButtonNum = 1;
        HandlePresetButton(1);
      }
      else if (GPIO_Pin == btnPst2_Pin)
      {
        pressedButtonNum = 2;
        HandlePresetButton(2);
      }
      else if (GPIO_Pin == btnPst3_Pin)
      {
        pressedButtonNum = 3;
        HandlePresetButton(3);
      }
      else if (GPIO_Pin == btnPst4_Pin)
      {
        pressedButtonNum = 4;
        HandlePresetButton(4);
      }
      else if (GPIO_Pin == btnBankUp_Pin)
        HandleBankUpButton();
      else if (GPIO_Pin == btnBankDown_Pin)
        HandleBankDownButton();
      else if (GPIO_Pin == btnCtl_Pin)
      {
        isCtlButton = true;
        printf("[BTN] CTL pressed - hold for 2s to toggle tuner\r\n");
      }
      else if (GPIO_Pin == btnTapTempo_Pin)
      {
        isTapTempoButton = true;
        printf("[BTN] Tap Tempo pressed - hold for save (2s) or clear (5s) - time=%lu\r\n", 
               buttonPressTime);
      }
    }
  }
  /* Rising edge (button released) */
  else
  {
    /* Only process release for the button that was pressed */
    if (pressedButton == GPIO_Pin)
    {
      buttonState = BTN_STATE_DEBOUNCING;
      debounceStartTime = HAL_GetTick();
    }
  }
}

/**
  * @brief  Process button timing and hold detection (call from main loop)
  * @retval None
  */
void PresetButtons_Process(void)
{
  uint32_t currentTime = HAL_GetTick();
  
  /* Handle button hold detection - only for Tap Tempo button */
  if (buttonState == BTN_STATE_PRESSED && isTapTempoButton)
  {
    uint32_t holdTime = currentTime - buttonPressTime;
    
    /* Check for save action (2 seconds) */
    if (holdTime >= HOLD_TIME_SAVE_MS && !saveActionTriggered)
    {
      saveActionTriggered = true;
      
      /* Save current GP-5 preset to current STM32 location */
      printf("[BTN] Tap Tempo SAVE (2s): Requesting current GP-5 preset...\r\n");
      GP5_MIDI_RequestPresetNumber();
      /* Actual save happens in PresetButtons_SetCurrentPreset() */
    }
    
    /* Check for clear action (5+ seconds) - independent check, can override save */
    if (holdTime >= HOLD_TIME_CLEAR_MS && !clearActionTriggered)
    {
      clearActionTriggered = true;
      
      /* Clear the current STM32 preset location */
      uint8_t presetIndex = CalculatePresetIndex(currentBank, currentButton);
      
      printf("[BTN] Tap Tempo CLEAR (5s): Bank %d, Button %d\r\n", 
             currentBank + 1, currentButton + 1);
      
      if (FlashStorage_SavePreset(presetIndex, 0xFF) == 0)
      {
        /* Update display to show "--" */
        Display_GP5SavedPreset(0xFF);
        printf("[BTN] Preset cleared successfully\r\n");
      }
    }
  }
  
  /* Handle button hold detection - only for CTL button */
  if (buttonState == BTN_STATE_PRESSED && isCtlButton)
  {
    uint32_t holdTime = currentTime - buttonPressTime;
    
    /* Check for tuner toggle at 2 seconds */
    if (holdTime >= HOLD_TIME_SAVE_MS && !tunerActionTriggered)
    {
      tunerActionTriggered = true;  /* Mark that tuner action happened */
      
      /* Toggle tuner state */
      tunerOn = !tunerOn;
      
      /* Send CC#58 (Tuner ON/OFF) to GP-5 */
      uint8_t value = tunerOn ? 0x7F : 0x00;  /* 127 for ON, 0 for OFF */
      printf("[BTN] CTL (2s hold): Tuner %s (CC#58, value=%d)\r\n", tunerOn ? "ON" : "OFF", value);
      MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_TUNER, value);
    }
  }
  
  /* Handle debouncing */
  if (buttonState == BTN_STATE_DEBOUNCING)
  {
    uint32_t elapsed = currentTime - debounceStartTime;
    
    if (elapsed >= DEBOUNCE_TIME_MS)
    {
      /* Handle Tap Tempo button (quick press - not held for 2s) */
      if (isTapTempoButton && !saveActionTriggered && !clearActionTriggered)
      {
        uint32_t currentTime = HAL_GetTick();
        
        if (lastTapTime != 0)
        {
          /* Calculate time since last tap */
          uint32_t timeDiff = currentTime - lastTapTime;
          
          /* Check if within valid tempo range */
          if (timeDiff >= TAP_TEMPO_MIN_MS && timeDiff <= TAP_TEMPO_MAX_MS)
          {
            tempoMs = timeDiff;
            printf("[BTN] Tap Tempo: %lu ms\r\n", tempoMs);
            SendTapTempoSysEx(tempoMs);
          }
          else
          {
            printf("[BTN] Tap Tempo: Out of range (%lu ms), valid range: %d-%d ms\r\n", 
                   timeDiff, TAP_TEMPO_MIN_MS, TAP_TEMPO_MAX_MS);
          }
        }
        else
        {
          printf("[BTN] Tap Tempo: First tap registered\r\n");
        }
        
        lastTapTime = currentTime;
      }
      
      /* If CTL button was released without tuner action, send CTL screen command */
      /* But only if tuner is OFF - if tuner is ON, block CTL command */
      if (isCtlButton && !tunerActionTriggered && !tunerOn)
      {
        printf("[BTN] CTL (quick press): Sending CC#69 (CTL screen)\r\n");
        MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, GP5_CC_CTL_SCREEN, 0x7F);
      }
      else if (isCtlButton && !tunerActionTriggered && tunerOn)
      {
        printf("[BTN] CTL blocked: Tuner is ON - hold 2s to turn tuner OFF\r\n");
      }
      
      /* Debounce complete, return to idle */
      buttonState = BTN_STATE_IDLE;
      pressedButton = 0;
      pressedButtonNum = 0;
      isTapTempoButton = false;
      isCtlButton = false;
      commandSentThisPress = false;
      saveActionTriggered = false;
      clearActionTriggered = false;
      tunerActionTriggered = false;
    }
  }
  
  /* Handle preset recall retry logic */
  if (presetRecallRetryCount > 0 && presetRecallRetryCount <= PRESET_RETRY_COUNT)
  {
    uint32_t elapsed = currentTime - lastPresetRecallTime;
    if (elapsed >= PRESET_RETRY_DELAY_MS)
    {
      /* Retry sending preset */
      uint8_t presetIndex = CalculatePresetIndex(currentBank, currentButton);
      uint8_t storedGP5Preset = FlashStorage_GetPreset(presetIndex);
      
      if (storedGP5Preset != 0xFF && storedGP5Preset <= MAX_PRESET_NUMBER)
      {
        printf("[PresetButtons] Retry %d/%d: Sending preset %d\r\n", 
               presetRecallRetryCount, PRESET_RETRY_COUNT, storedGP5Preset);
        
        GP5_MIDI_NotifyPresetChangeSent();
        MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, 0, storedGP5Preset);
        
        lastPresetRecallTime = currentTime;
        presetRecallRetryCount++;
        
        if (presetRecallRetryCount > PRESET_RETRY_COUNT)
        {
          printf("[PresetButtons] Preset recall failed after %d retries\r\n", PRESET_RETRY_COUNT);
          presetRecallRetryCount = 0;
        }
      }
      else
      {
        presetRecallRetryCount = 0;
      }
    }
  }
}

/**
  * @brief  Get current bank number
  * @retval Current bank (0-15)
  */
uint8_t PresetButtons_GetCurrentBank(void)
{
  return currentBank;
}

/**
  * @brief  Get current GP-5 preset number
  * @retval Current preset (0-99)
  */
uint8_t PresetButtons_GetCurrentPreset(void)
{
  return currentGP5Preset;
}

/**
  * @brief  Set current GP-5 preset number (called when GP-5 reports preset)
  * @param  preset: GP-5 Preset number (0-99)
  * @retval None
  */
void PresetButtons_SetCurrentPreset(uint8_t preset)
{
  if (preset > MAX_PRESET_NUMBER)
  {
    printf("[PresetButtons] Invalid GP-5 preset: %d\r\n", preset);
    return;
  }
  
  currentGP5Preset = preset;
  currentPresetValid = true;
  
  printf("[PresetButtons] GP-5 preset updated: %d\r\n", preset);
  
  /* Check if this was triggered by a save action (Tap Tempo) */
  if (saveActionTriggered)
  {
    /* Save the current GP-5 preset to the current STM32 preset location */
    uint8_t presetIndex = CalculatePresetIndex(currentBank, currentButton);
    
    printf("[BTN] SAVING GP-5 preset %d to STM32 Bank %d, Button %d\r\n",
           preset, currentBank + 1, currentButton + 1);
    
    if (FlashStorage_SavePreset(presetIndex, preset) == 0)
    {
      /* Update display */
      Display_GP5SavedPreset(preset);
      printf("[BTN] Preset saved successfully\r\n");
    }
    
    /* Note: saveActionTriggered flag stays true until button is released */
    /* This prevents repeated saves while button is still held */
  }
  
  /* Clear retry counter if preset received successfully */
  if (presetRecallRetryCount > 0)
  {
    printf("[PresetButtons] Preset recall ACK received\r\n");
    presetRecallRetryCount = 0;
  }
}

/**
  * @brief  Notify that a preset change ACK was received
  * @param  sentByUs: true if we sent the command, false if external
  * @retval None
  */
void PresetButtons_OnPresetChangeACK(bool sentByUs)
{
  if (sentByUs)
  {
    /* We sent the command, ACK confirms it */
    /* Request preset to confirm the change */
    GP5_MIDI_RequestPresetNumber();
    
    /* Clear retry counter */
    presetRecallRetryCount = 0;
  }
  else
  {
    /* External preset change, request current preset */
    printf("[PresetButtons] External change detected, requesting preset\r\n");
    GP5_MIDI_RequestPresetNumber();
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Calculate STM32 preset index from bank and button number
  * @param  bank: Bank number (0-15)
  * @param  button: Button number (0-4)
  * @retval Preset index (0-79)
  */
static uint8_t CalculatePresetIndex(uint8_t bank, uint8_t button)
{
  return FlashStorage_CalculateIndex(bank, button);
}

/**
  * @brief  Update display for current STM32 preset
  * @retval None
  */
static void UpdateDisplayForCurrentPreset(void)
{
  /* Display bank number (1-16 for user) */
  Display_BankNumber(currentBank + 1, White);
  
  /* Display button number (1-5 for user) */
  Display_PresetNumber(currentButton + 1);
  
  /* Display stored GP-5 preset for this location */
  uint8_t presetIndex = CalculatePresetIndex(currentBank, currentButton);
  uint8_t storedGP5Preset = FlashStorage_GetPreset(presetIndex);
  Display_GP5SavedPreset(storedGP5Preset);
}

/**
  * @brief  Send preset with retry mechanism
  * @param  gp5Preset: GP-5 preset number to send
  * @retval None
  */
static void SendPresetWithRetry(uint8_t gp5Preset)
{
  printf("[PresetButtons] Sending GP-5 preset: %d\r\n", gp5Preset);
  
  GP5_MIDI_NotifyPresetChangeSent();
  MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, 0, gp5Preset);
  
  /* Initialize retry mechanism */
  lastPresetRecallTime = HAL_GetTick();
  presetRecallRetryCount = 1;
}

/**
  * @brief  Handle preset button press (btnPst0 to btnPst4)
  * @param  buttonNum: Button number (0-4)
  * @retval None
  */
static void HandlePresetButton(uint8_t buttonNum)
{
  if (buttonNum >= PRESETS_PER_BANK)
    return;
  
  /* Use temporary bank if bank was changed, otherwise use current bank */
  uint8_t bankToUse = isBankInverted ? temporaryBank : currentBank;
  
  /* Get stored GP-5 preset for this STM32 preset */
  uint8_t presetIndex = CalculatePresetIndex(bankToUse, buttonNum);
  uint8_t storedGP5Preset = FlashStorage_GetPreset(presetIndex);
  
  /* Display the button number */
  Display_PresetNumber(buttonNum + 1);  /* Display 1-5 for user */
  
  /* Display the stored GP-5 preset */
  Display_GP5SavedPreset(storedGP5Preset);
  
  /* Update current position */
  currentBank = bankToUse;
  currentButton = buttonNum;
  
  /* Normalize bank display if it was inverted (regardless of preset validity) */
  if (isBankInverted)
  {
    extern SSD1306_COLOR bankNumColor;
    bankNumColor = InvertBankBackground(bankNumColor, currentBank + 1);
    isBankInverted = false;
    printf("[BTN] Bank display normalized after preset selection\r\n");
  }
  
  /* Save current preset index to Flash */
  FlashStorage_SaveCurrentPresetIndex(presetIndex);
  
  /* Only send CC if preset is valid (not 0xFF) */
  if (storedGP5Preset == 0xFF)
  {
    printf("[BTN] Preset %d (Bank %d, Button %d) is empty, no CC sent\r\n",
           presetIndex, bankToUse + 1, buttonNum + 1);
    return;
  }
  
  /* Send CC if preset is different or bank changed */
  if (!currentPresetValid || storedGP5Preset != currentGP5Preset || isBankInverted)
  {
    printf("[BTN] Preset %d: Sending GP-5 #%d\r\n", presetIndex, storedGP5Preset);
    SendPresetWithRetry(storedGP5Preset);
    
    commandSentThisPress = true;
    
    /* Optimistically update current preset */
    currentGP5Preset = storedGP5Preset;
    currentPresetValid = true;
  }
}

/**
  * @brief  Handle bank up button press
  * @retval None
  */
static void HandleBankUpButton(void)
{
  /* Calculate new temporary bank */
  uint8_t newTempBank = isBankInverted ? temporaryBank : currentBank;
  
  /* Wrap around: if at max (15), go to 0 */
  if (newTempBank >= (NUM_BANKS - 1))
    newTempBank = 0;
  else
    newTempBank++;
  
  temporaryBank = newTempBank;
  
  printf("[BTN] Bank %d (temp, current=%d)\r\n", temporaryBank + 1, currentBank + 1);
  
  /* Update display */
  extern SSD1306_COLOR bankNumColor;
  
  if (temporaryBank == currentBank)
  {
    /* Temporary bank equals current bank - revert to normal if inverted */
    if (isBankInverted)
    {
      bankNumColor = InvertBankBackground(bankNumColor, currentBank + 1);
      isBankInverted = false;
      printf("[BTN] Bank display normalized\r\n");
    }
  }
  else
  {
    /* Temporary bank differs from current - invert display */
    if (!isBankInverted)
    {
      /* First time inverting */
      bankNumColor = InvertBankBackground(bankNumColor, temporaryBank + 1);
      isBankInverted = true;
    }
    else
    {
      /* Already inverted, just update the number */
      Display_BankNumber(temporaryBank + 1, bankNumColor);
    }
  }
}

/**
  * @brief  Handle bank down button press
  * @retval None
  */
static void HandleBankDownButton(void)
{
  /* Calculate new temporary bank */
  uint8_t newTempBank = isBankInverted ? temporaryBank : currentBank;
  
  /* Wrap around: if at min (0), go to max (15) */
  if (newTempBank == 0)
    newTempBank = NUM_BANKS - 1;
  else
    newTempBank--;
  
  temporaryBank = newTempBank;
  
  printf("[BTN] Bank %d (temp, current=%d)\r\n", temporaryBank + 1, currentBank + 1);
  
  /* Update display */
  extern SSD1306_COLOR bankNumColor;
  
  if (temporaryBank == currentBank)
  {
    /* Temporary bank equals current bank - revert to normal if inverted */
    if (isBankInverted)
    {
      bankNumColor = InvertBankBackground(bankNumColor, currentBank + 1);
      isBankInverted = false;
      printf("[BTN] Bank display normalized\r\n");
    }
  }
  else
  {
    /* Temporary bank differs from current - invert display */
    if (!isBankInverted)
    {
      /* First time inverting */
      bankNumColor = InvertBankBackground(bankNumColor, temporaryBank + 1);
      isBankInverted = true;
    }
    else
    {
      /* Already inverted, just update the number */
      Display_BankNumber(temporaryBank + 1, bankNumColor);
    }
  }
}

/**
  * @brief  Calculate CRC8 checksum
  * @param  data: Pointer to data buffer
  * @param  length: Length of data
  * @retval CRC8 checksum
  */
static uint8_t CalculateCRC8(const uint8_t* data, uint32_t length)
{
  uint8_t crc = 0x00;
  
  for (uint32_t i = 0; i < length; i++)
  {
    uint8_t cur = data[i] & 0xFF;
    crc ^= cur;
    
    for (uint8_t j = 0; j < 8; j++)
    {
      if ((crc & 0x80) != 0)
      {
        crc = ((crc << 1) ^ 0x07) & 0xFF;
      }
      else
      {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  
  return crc & 0xFF;
}

/**
  * @brief  Convert float to hex bytes (little endian)
  * @param  value: Float value to convert
  * @param  bytes: Output buffer (4 bytes)
  * @retval None
  */
static void FloatToHexBytes(float value, uint8_t* bytes)
{
  union {
    float f;
    uint8_t u[4];
  } converter;
  
  converter.f = value;
  
  /* Little endian format - just copy the bytes directly */
  bytes[0] = converter.u[0];
  bytes[1] = converter.u[1];
  bytes[2] = converter.u[2];
  bytes[3] = converter.u[3];
}

/**
  * @brief  Send Tap Tempo SysEx message to GP-5
  * @param  tempoMs: Tempo value in milliseconds
  * @retval None
  */
static void SendTapTempoSysEx(uint32_t tempoMs)
{
  uint8_t sysexBuffer[128];
  uint8_t codeBuffer[32];
  uint32_t codeIdx = 0;
  
  /* Fixed header bytes (before nibble split): 01 00 0E 11 48 */
  codeBuffer[codeIdx++] = 0x01;
  codeBuffer[codeIdx++] = 0x00;
  codeBuffer[codeIdx++] = 0x0E;
  codeBuffer[codeIdx++] = 0x11;
  codeBuffer[codeIdx++] = 0x48;
  
  /* Block = 7 */
  codeBuffer[codeIdx++] = 0x07;
  
  /* 000000 */
  codeBuffer[codeIdx++] = 0x00;
  codeBuffer[codeIdx++] = 0x00;
  codeBuffer[codeIdx++] = 0x00;
  
  /* Nparam = 1 */
  codeBuffer[codeIdx++] = 0x01;
  
  /* 000000 */
  codeBuffer[codeIdx++] = 0x00;
  codeBuffer[codeIdx++] = 0x00;
  codeBuffer[codeIdx++] = 0x00;
  
  /* Convert tempo to float and get hex bytes */
  float tempoFloat = (float)tempoMs;
  uint8_t floatBytes[4];
  FloatToHexBytes(tempoFloat, floatBytes);
  
  codeBuffer[codeIdx++] = floatBytes[0];
  codeBuffer[codeIdx++] = floatBytes[1];
  codeBuffer[codeIdx++] = floatBytes[2];
  codeBuffer[codeIdx++] = floatBytes[3];
  
  /* Calculate CRC8 checksum */
  uint8_t checksum = CalculateCRC8(codeBuffer, codeIdx);
  
  /* Build SysEx with nibble splitting */
  /* Each byte is split: high nibble (upper 4 bits), then low nibble (lower 4 bits) */
  uint32_t sysexIdx = 0;
  sysexBuffer[sysexIdx++] = 0xF0;  /* SysEx start */
  
  /* Add checksum split into nibbles */
  sysexBuffer[sysexIdx++] = (checksum >> 4) & 0x0F;  /* High nibble */
  sysexBuffer[sysexIdx++] = checksum & 0x0F;          /* Low nibble */
  
  /* Add code bytes, each split into nibbles */
  for (uint32_t i = 0; i < codeIdx; i++)
  {
    sysexBuffer[sysexIdx++] = (codeBuffer[i] >> 4) & 0x0F;  /* High nibble */
    sysexBuffer[sysexIdx++] = codeBuffer[i] & 0x0F;         /* Low nibble */
  }
  
  sysexBuffer[sysexIdx++] = 0xF7;  /* SysEx end */
  
  printf("[Tap Tempo] Tempo = %lu ms (%.1f BPM)\r\n", tempoMs, 60000.0f / (float)tempoMs);
  printf("[SysEx] ");
  for (uint32_t i = 0; i < sysexIdx; i++)
  {
    printf("%02X ", sysexBuffer[i]);
  }
  printf("\r\n");
  
  MIDI_Manager_SendSysEx(sysexBuffer, sysexIdx);
}
