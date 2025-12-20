/**
  ******************************************************************************
  * @file    gp5_midi.h
  * @author  Custom Implementation
  * @brief   GP-5 Pedal MIDI Communication Interface
  ******************************************************************************
  */

#ifndef GP5_MIDI_H
#define GP5_MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "button_handler.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* GP-5 MIDI Control Change Numbers */
#define GP5_CC_PATCH_DOWN           24
#define GP5_CC_PATCH_UP             25
#define GP5_CC_TAP_TEMPO            64

/* GP-5 Patch Control CC Numbers (48-57) */
#define GP5_CC_NR                   48  /* Noise Reduction */
#define GP5_CC_PRE                  49  /* Preamp */
#define GP5_CC_DST                  50  /* Distortion */
#define GP5_CC_NS                   51  /* Neural Amp */
#define GP5_CC_AMP                  52  /* Amplifier */
#define GP5_CC_CAB                  53  /* Cabinet */
#define GP5_CC_EQ                   54  /* Equalizer */
#define GP5_CC_MOD                  55  /* Modulation */
#define GP5_CC_DLY                  56  /* Delay */
#define GP5_CC_RVB                  57  /* Reverb */

/* GP-5 Screen Control */
#define GP5_CC_CTL_SCREEN           69  /* CTL screen toggle (0x45) - forces app refresh */

/* GP-5 MIDI Channel (0-15, where 0 = MIDI Ch 1) */
#define GP5_MIDI_CHANNEL            0

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize GP-5 MIDI messages for all buttons
  * @retval None
  */
void GP5_MIDI_Init(void);

/**
  * @brief  Request initial preset number from GP-5 (call after enumeration)
  * @retval None
  */
void GP5_MIDI_RequestInitialPreset(void);

/**
  * @brief  Test function: Save a test scene (for development)
  * @param  preset: Preset number
  * @param  sceneNum: Scene number (1-3)
  * @retval None
  */
void GP5_MIDI_SaveTestScene(uint8_t preset, uint8_t sceneNum);

/**
  * @brief  Handle button events and send appropriate GP-5 MIDI messages
  * @param  button: Button ID
  * @param  event: Button event type
  * @retval None
  */
void GP5_MIDI_HandleButtonEvent(ButtonID_t button, ButtonEvent_t event);

/**
  * @brief  Process received MIDI messages from GP-5
  * @param  data: Pointer to USB-MIDI packet data
  * @param  length: Length of data in bytes
  * @retval None
  */
void GP5_MIDI_ProcessReceivedData(uint8_t *data, uint16_t length);

/**
  * @brief  Request preset number from GP-5
  * @retval None
  */
void GP5_MIDI_RequestPresetNumber(void);

/**
  * @brief  Parse preset number response (18 bytes)
  * @param  data: Pointer to clean MIDI data
  * @param  len: Length of data
  * @param  preset: Pointer to store preset number (0-99)
  * @retval true if valid preset number response, false otherwise
  */
bool GP5_MIDI_ParsePresetNumber(uint8_t *data, uint16_t len, uint8_t *preset);

/**
  * @brief  Detect preset change ACK (22 bytes)
  * @param  data: Pointer to clean MIDI data
  * @param  len: Length of data
  * @retval true if preset change ACK detected, false otherwise
  */
bool GP5_MIDI_IsPresetChangeACK(uint8_t *data, uint16_t len);

/**
  * @brief  Request patch info from GP-5
  * @retval None
  */
void GP5_MIDI_RequestPatchInfo(void);

/**
  * @brief  Parse patch info response (4th message, 48 bytes)
  * @param  data: Pointer to clean MIDI data
  * @param  len: Length of data
  * @param  patchBitmap: Pointer to store 32-bit patch bitmap
  * @retval true if valid patch info (4th message), false otherwise
  */
bool GP5_MIDI_ParsePatchInfo(uint8_t *data, uint16_t len, uint32_t *patchBitmap);

/**
  * @brief  Set patch on/off state
  * @param  patchCC: CC number (48-57)
  * @param  turnOn: true to turn on, false to turn off
  * @retval None
  */
void GP5_MIDI_SetPatchState(uint8_t patchCC, bool turnOn);

#ifdef __cplusplus
}
#endif

#endif /* GP5_MIDI_H */
