/**
  ******************************************************************************
  * @file    preset_buttons.c
  * @author  Custom Implementation
  * @brief   Preset and Bank Button Handler Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "preset_buttons.h"
#include "midi_manager.h"
#include "gp5_midi.h"
#include "display.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  BTN_STATE_IDLE = 0,
  BTN_STATE_PRESSED,
  BTN_STATE_DEBOUNCING
} ButtonState_t;

/* Private define ------------------------------------------------------------*/
#define DEBOUNCE_TIME_MS    20

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t currentBank = 0;              /* Current bank (0-19) */
static uint8_t temporaryBank = 0;            /* Temporary bank when bank up/down pressed */
static uint8_t currentPreset = 0;            /* Current GP-5 preset (0-99) */
static bool currentPresetValid = false;      /* Is current preset known? */
static bool isBankInverted = false;          /* Is bank display inverted? */

static ButtonState_t buttonState = BTN_STATE_IDLE;
static uint16_t pressedButton = 0;           /* Which button is pressed */
static uint32_t debounceStartTime = 0;       /* When debounce started */
static bool commandSentThisPress = false;    /* Did we send a command? */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void HandlePresetButton(uint8_t buttonNum);
static void HandleBankUpButton(void);
static void HandleBankDownButton(void);
static uint8_t CalculatePresetNumber(uint8_t bank, uint8_t buttonNum);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize preset button handler
  * @retval None
  */
void PresetButtons_Init(void)
{
  currentBank = 0;
  temporaryBank = 0;
  currentPreset = 0;
  currentPresetValid = false;
  isBankInverted = false;
  buttonState = BTN_STATE_IDLE;
  pressedButton = 0;
  commandSentThisPress = false;
  
  /* Enable and configure NVIC for EXTI interrupts */
  /* EXTI0_1 covers btnPst0 (PB0) and btnPst1 (PB1) */
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
  
  /* EXTI2_3 covers btnPst2 (PB2) and btnPst3 (PB3) */
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
  
  /* EXTI4_15 covers btnPst4 (PB4), btnBankUp (PB5), btnBankDown (PB6), btnCtl (PB7), btnTapTempo (PB8) */
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
  
  printf("[PresetButtons] Initialized - Bank: 0, Preset: Unknown\r\n");
  printf("[PresetButtons] NVIC enabled for button interrupts\r\n");
}

/**
  * @brief  GPIO EXTI callback for button interrupts
  * @param  GPIO_Pin: Pin number that triggered the interrupt
  * @retval None
  */
void PresetButtons_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Read current pin state */
  GPIO_PinState pinState;
  
  /* Determine which port to read from (all buttons are on GPIOB) */
  pinState = HAL_GPIO_ReadPin(GPIOB, GPIO_Pin);
  
  /* Falling edge (button pressed) - active low with pullup */
  if (pinState == GPIO_PIN_RESET)
  {
    /* Only process if no button is currently pressed */
    if (buttonState == BTN_STATE_IDLE)
    {
      buttonState = BTN_STATE_PRESSED;
      pressedButton = GPIO_Pin;
      commandSentThisPress = false;
      
      /* Handle button press immediately */
      if (GPIO_Pin == btnPst0_Pin)
        HandlePresetButton(0);
      else if (GPIO_Pin == btnPst1_Pin)
        HandlePresetButton(1);
      else if (GPIO_Pin == btnPst2_Pin)
        HandlePresetButton(2);
      else if (GPIO_Pin == btnPst3_Pin)
        HandlePresetButton(3);
      else if (GPIO_Pin == btnPst4_Pin)
        HandlePresetButton(4);
      else if (GPIO_Pin == btnBankUp_Pin)
        HandleBankUpButton();
      else if (GPIO_Pin == btnBankDown_Pin)
        HandleBankDownButton();
      else if (GPIO_Pin == btnCtl_Pin)
        printf("[BTN] CTL pressed (TODO: implement later)\r\n");
      else if (GPIO_Pin == btnTapTempo_Pin)
        printf("[BTN] Tap Tempo pressed (TODO: implement later)\r\n");
    }
  }
  /* Rising edge (button released) */
  else
  {
    /* Only process release for the button that was pressed */
    if (buttonState == BTN_STATE_PRESSED && pressedButton == GPIO_Pin)
    {
      buttonState = BTN_STATE_DEBOUNCING;
      debounceStartTime = HAL_GetTick();
    }
  }
}

/**
  * @brief  Process button debounce timing (call from main loop)
  * @retval None
  */
void PresetButtons_Process(void)
{
  if (buttonState == BTN_STATE_DEBOUNCING)
  {
    uint32_t elapsed = HAL_GetTick() - debounceStartTime;
    
    if (elapsed >= DEBOUNCE_TIME_MS)
    {
      /* Debounce complete, return to idle */
      buttonState = BTN_STATE_IDLE;
      pressedButton = 0;
      commandSentThisPress = false;
    }
  }
}

/**
  * @brief  Get current bank number
  * @retval Current bank (0-19)
  */
uint8_t PresetButtons_GetCurrentBank(void)
{
  return currentBank;
}

/**
  * @brief  Get current preset number
  * @retval Current preset (0-99)
  */
uint8_t PresetButtons_GetCurrentPreset(void)
{
  return currentPreset;
}

/**
  * @brief  Set current preset number (called when GP-5 reports preset)
  * @param  preset: Preset number (0-99)
  * @retval None
  */
void PresetButtons_SetCurrentPreset(uint8_t preset)
{
  if (preset > MAX_PRESET_NUMBER)
  {
    printf("[PresetButtons] Invalid preset: %d\r\n", preset);
    return;
  }
  
  uint8_t oldPreset = currentPreset;
  currentPreset = preset;
  currentPresetValid = true;
  
  /* Calculate which bank this preset belongs to */
  uint8_t calculatedBank = preset / PRESETS_PER_BANK;
  currentBank = calculatedBank;
  temporaryBank = calculatedBank;
  
  /* Display preset number (button number 0-4) */
  Display_PresetNumber(preset);
  
  /* Display bank number with normal color (white on black) */
  if (isBankInverted)
  {
    /* Invert back to normal */
    extern SSD1306_COLOR bankNumColor;
    bankNumColor = InvertBankBackground(bankNumColor, currentBank);
    isBankInverted = false;
  }
  else
  {
    /* Display with white color */
    extern SSD1306_COLOR bankNumColor;
    bankNumColor = White;
    Display_BankNumber(currentBank, bankNumColor);
  }
  
  printf("[PresetButtons] Preset updated: %d -> %d (Bank %d)\r\n", oldPreset, preset, calculatedBank);
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
    if (isBankInverted)
    {
      /* Bank was changed, request current preset and normalize display */
      printf("[PresetButtons] Preset change ACK, requesting preset to confirm\r\n");
      GP5_MIDI_RequestPresetNumber();
      /* Display will be normalized when preset is received */
    }
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
  * @brief  Calculate preset number from bank and button number
  * @param  bank: Bank number (0-19)
  * @param  buttonNum: Button number (0-4)
  * @retval Preset number (0-99)
  */
static uint8_t CalculatePresetNumber(uint8_t bank, uint8_t buttonNum)
{
  return (bank * PRESETS_PER_BANK) + buttonNum;
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
  
  /* Calculate target preset number */
  uint8_t targetPreset = CalculatePresetNumber(bankToUse, buttonNum);
  
  /* Only send command if preset will actually change */
  if (currentPresetValid && targetPreset == currentPreset)
    return;
  
  /* Send CC#0 with preset number */
  printf("[BTN] Preset %d\r\n", targetPreset);
  GP5_MIDI_NotifyPresetChangeSent();  /* Notify before sending */
  MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, 0, targetPreset);
  
  commandSentThisPress = true;
  
  /* Optimistically update current preset */
  currentPreset = targetPreset;
  currentPresetValid = true;
  
  /* Update display with new preset number */
  Display_PresetNumber(targetPreset);
  
  /* Note: Bank display will be normalized when ACK is received and preset confirmed */
}

/**
  * @brief  Handle bank up button press
  * @retval None
  */
static void HandleBankUpButton(void)
{
  /* Calculate new temporary bank */
  uint8_t newTempBank = isBankInverted ? temporaryBank : currentBank;
  
  if (newTempBank >= (NUM_BANKS - 1))
    return;
  
  newTempBank++;
  temporaryBank = newTempBank;
  
  printf("[BTN] Bank %d (temp, current=%d)\r\n", temporaryBank, currentBank);
  
  /* Update display */
  extern SSD1306_COLOR bankNumColor;
  
  if (temporaryBank == currentBank)
  {
    /* Temporary bank equals current bank - revert to normal if inverted */
    if (isBankInverted)
    {
      bankNumColor = InvertBankBackground(bankNumColor, currentBank);
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
      bankNumColor = InvertBankBackground(bankNumColor, temporaryBank);
      isBankInverted = true;
    }
    else
    {
      /* Already inverted, just update the number */
      Display_BankNumber(temporaryBank, bankNumColor);
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
  
  if (newTempBank == 0)
    return;
  
  newTempBank--;
  temporaryBank = newTempBank;
  
  printf("[BTN] Bank %d (temp, current=%d)\r\n", temporaryBank, currentBank);
  
  /* Update display */
  extern SSD1306_COLOR bankNumColor;
  
  if (temporaryBank == currentBank)
  {
    /* Temporary bank equals current bank - revert to normal if inverted */
    if (isBankInverted)
    {
      bankNumColor = InvertBankBackground(bankNumColor, currentBank);
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
      bankNumColor = InvertBankBackground(bankNumColor, temporaryBank);
      isBankInverted = true;
    }
    else
    {
      /* Already inverted, just update the number */
      Display_BankNumber(temporaryBank, bankNumColor);
    }
  }
}
